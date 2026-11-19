"""Checks the logs and screenshots of the slime live run (called by Scripts/run_live_slime.sh).

  check_live_slime.py <project root>

Like check_live.py, the expected numbers are worked out here from data/values.json with the
formulas of the design. Exit code 0 = every check passed.
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from check_live import Checker, Log, angle_diff  # noqa: E402


class SlimeLog(Log):
    def __init__(self, path):
        super().__init__(path)
        # Events that only this run uses.
        from check_live import parse_fields
        for line in self.lines:
            for kind in ("HydraBolt", "HydraCast", "HydraBurn", "HydraField"):
                marker = kind + " "
                if marker in line and "exec '" not in line:
                    fields = parse_fields(line[line.index(marker) + len(marker):])
                    for key in ("attacker", "target", "who"):
                        if isinstance(fields.get(key), float):
                            fields[key] = str(int(fields[key]))
                    self.events.setdefault(kind, []).append(fields)


def main(root):
    with open(os.path.join(root, "data", "values.json"), encoding="utf-8") as f:
        v = json.load(f)
    slime, human = v["forms"]["slime"], v["forms"]["humanoid"]
    slam, sword = v["weapons"]["slime_slam"], v["weapons"]["longsword"]
    bolt, field, burn = v["bolt"], v["fire_field"], v["burning"]
    slam_ratio = slam["weight"] / slime["strength"]
    slam_light_cost = v["attack"]["light_stamina_per_weight_ratio"] * slam_ratio
    slam_heavy_cost = v["attack"]["heavy_stamina_per_weight_ratio"] * slam_ratio
    slime_weight = slime["body_weight"] + slam["weight"]
    tr = v["turn_rate"]
    slime_turn = max(min(tr["base_deg_s"] / ((slime_weight / slime["strength"]) ** tr["weight_strength_exponent"]), tr["max_deg_s"]), tr["min_deg_s"])
    interval = burn["stack_interval_ms"] / 1000.0

    host = SlimeLog(os.path.join(root, "Saved", "Logs", "SlimeHost.log"))
    client = SlimeLog(os.path.join(root, "Saved", "Logs", "SlimeClient.log"))
    c = Checker()

    def val(log, t, char_id, key):
        s = log.stats(t, char_id)
        return s[key] if s else None

    def one(name, events):
        c.check(name, len(events) == 1, "%d found" % len(events))
        return events[0] if events else {}

    print("--- the slime as a playable form (model from the Blender pipeline)")
    if host.own_id is None or client.own_id is None:
        c.check("both windows know their own character", False)
        print("passed=%d failed=%d" % (c.passed, c.failed))
        return 1
    H, C, AI = host.own_id, client.own_id, "ai"
    c.check("host is the slime (start option ?Form=slime), client and AI are humanoids",
            (host.stats(1, H) or {}).get("form") == "slime" and (host.stats(1, C) or {}).get("form") == "humanoid" and (host.stats(1, AI) or {}).get("form") == "humanoid")
    c.check("client sees the host as a slime", (client.stats(1, H) or {}).get("form") == "slime")
    c.check("the slime uses its model with all eleven animations", host.count("Form slime uses the model slime with 11 animations") >= 1 and client.count("Form slime uses the model slime with 11 animations") >= 1)
    c.check("no error lines from the game in either log", host.count("LogHydra: Error") + client.count("LogHydra: Error") == 0)
    c.near("slime health from values.json", val(host, 1, H, "hp"), slime["hp_max"], 0.01)
    c.near("slime: little stamina", val(host, 1, H, "stamina"), slime["stamina_max"], 0.01)
    c.near("slime: a lot of mana", val(host, 1, H, "mana"), slime["mana_max"], 0.01)
    c.near("the humanoid has no mana bar", val(host, 1, C, "maxmana"), 0, 0.01)
    c.near("client sees the slime's mana (replicated)", val(client, 1, H, "mana"), slime["mana_max"], 0.01)
    c.near("the slime's body is low: its centre is half its height above the floor", (val(host, 1, C, "z") or 0) - (val(host, 1, H, "z") or 0),
           (human["body_height_m"] - slime["body_height_m"]) * 50, 3)
    cs = [e for e in host.events.get("HydraCombatStats", []) if e.get("form") == "slime"]
    c.near("slime turn rate from its weight / strength (capped)", cs[0].get("turn") if cs else None, slime_turn, 0.2)

    print("--- slam (melee, stamina)")
    s1 = one("light slam hits the AI", host.find("HydraHit", 2.5, 3.2, attacker=H, target=AI))
    c.near("slam damage (blunt, no resistance on the humanoid)", s1.get("dmg"), slam["light"]["damage"], 0.01)
    c.near("lands after the slam wind-up", (s1.get("t") or 0) - 2.5, slam["light"]["windup_ms"] / 1000.0, 0.15)
    c.near("slam costs stamina by its weight", s1.get("attacker_stamina"), slime["stamina_max"] - slam_light_cost, 0.01)
    c.near("client sees the AI's health after the slam", val(client, 3.2, AI, "hp"), human["hp_max"] - slam["light"]["damage"], 0.01)
    s2 = one("charged slam hits", host.find("HydraHit", 4.0, 5.7, attacker=H, target=AI))
    c.check("it is the heavy slam", s2.get("kind") == "heavy")
    c.near("full charge damage", s2.get("dmg"), slam["heavy"]["damage"] * slam["heavy"]["full_charge_damage_factor"], 0.01)

    print("--- resistance: the slime shrugs off slash")
    r1 = one("the AI's sword hits the slime", host.find("HydraHit", 6.0, 6.8, attacker=AI, target=H))
    c.near("slash damage is reduced by the slime's resistance", r1.get("dmg"), sword["light"]["damage"] * (1 - slime["resistance"]["slash"]), 0.01)
    c.near("slime health after the hit", r1.get("target_hp"), slime["hp_max"] - sword["light"]["damage"] * (1 - slime["resistance"]["slash"]), 0.01)

    print("--- bolt without an element (mana)")
    cast1 = one("cast by a tap", host.find("HydraCast", 7.5, 8.2, attacker=H))
    c.near("bolt leaves after the cast wind-up", (cast1.get("t") or 0) - 7.5, bolt["windup_ms"] / 1000.0, 0.15)
    c.check("no charge, no fire", cast1.get("charge") == 0.0 and cast1.get("fire") == 0.0)
    c.near("base mana cost paid at the press (plus the regeneration during the wind-up)", cast1.get("mana"),
           slime["mana_max"] - bolt["mana_cost"] + slime["mana_regen_per_s"] * bolt["windup_ms"] / 1000.0, 1.0)
    b1 = one("the bolt hits the AI", host.find("HydraBolt", 7.5, 8.3, attacker=H, target=AI))
    c.check("plain hit", b1.get("result") == "hit" and b1.get("fire") == 0.0)
    c.near("bolt damage", b1.get("dmg"), bolt["damage"], 0.01)
    c.check("no burning from a bolt without fire", b1.get("burning") == 0.0)
    c.check("no fire field from a bolt without fire", len(host.find("HydraField", 7.5, 8.5)) == 0)

    c.check("charging is shown (host and client)", (host.stats(9.8, H) or {}).get("action") == "CastCharge" and True)
    cast2 = one("fully charged cast", host.find("HydraCast", 9.0, 10.6, attacker=H))
    c.near("released by itself at full charge", (cast2.get("t") or 0) - 9.0, bolt["max_charge_ms"] / 1000.0, 0.15)
    c.near("full charge", cast2.get("charge"), 1.0, 0.001)
    b2 = one("the charged bolt hits from 6 m", host.find("HydraBolt", 10.0, 10.9, attacker=H, target=AI))
    c.near("charging makes it stronger", b2.get("dmg"), bolt["damage"] * bolt["full_charge_damage_factor"], 0.01)
    c.near("flight time for the distance at bolt.speed_m_s", (b2.get("t") or 0) - (cast2.get("t") or 0), (600 - 35 - 65) / (bolt["speed_m_s"] * 100.0), 0.08)
    mana_before = val(host, 9.8, H, "mana")
    mana_after = val(host, 10.8, H, "mana")
    extra = bolt["mana_cost"] * (bolt["full_charge_cost_factor"] - 1)
    c.near("... and costlier (extra mana at release, minus a second of regeneration)",
           None if None in (mana_before, mana_after) else mana_before - mana_after, extra - slime["mana_regen_per_s"] * 1.0, 2.0)
    c.near("client sees the slime's mana go down", val(client, 10.8, H, "mana"), mana_after, slime["mana_regen_per_s"] * 0.35)

    print("--- fire bolt, fire field, burning")
    cast3 = one("fire bolt cast", host.find("HydraCast", 11.5, 12.2, attacker=H))
    c.check("the bolt carries fire once fire is awakened", cast3.get("fire") == 1.0)
    b3 = one("the fire bolt hits the AI", host.find("HydraBolt", 11.5, 12.6, attacker=H, target=AI))
    c.near("fire damage", b3.get("dmg"), bolt["damage"] * (1 - human["resistance"]["fire"]), 0.01)
    c.check("the hit adds burning stacks", b3.get("burning") == float(burn["stacks_per_hit"]))
    f1 = one("a fire field appears where the bolt landed", host.find("HydraField", 11.5, 12.6))
    c.near("field under the target (x)", f1.get("x"), 0, 5)
    c.near("field under the target (y)", f1.get("y"), 600, 5)
    c.near("field radius", f1.get("radius"), field["radius_m"] * 100, 0.5)
    c.near("field duration", f1.get("duration"), field["duration_ms"] / 1000.0, 0.01)
    c.check("AI burns: server and client see the stack", (host.stats(14, AI) or {}).get("burning") == 1.0 and (client.stats(14, AI) or {}).get("burning") == 1.0)
    burned_at_14 = None if val(host, 14, AI, "hp") is None else human["hp_max"] - bolt["damage"] - val(host, 14, AI, "hp")
    c.between("burning deals damage over time (about 1.9 s x damage_per_s_per_stack so far)", burned_at_14,
              burn["damage_per_s_per_stack"] * 1.0, burn["damage_per_s_per_stack"] * 2.5)
    in_field = [e for e in host.events.get("HydraBurn", []) if e.get("who") == C and e.get("reason") == "field"]
    step = one("the client steps into the field and burns at once", in_field)
    c.between("... right when it stepped in", step.get("t"), 12.6, 13.2)
    decay_ai = [e for e in host.events.get("HydraBurn", []) if e.get("who") == AI and e.get("reason") == "decay"]
    decay_c = [e for e in host.events.get("HydraBurn", []) if e.get("who") == C and e.get("reason") == "decay"]
    c.near("burning goes on after the field is gone: the AI's stack drops one interval after the hit",
           decay_ai[0].get("t") if decay_ai else None, (b3.get("t") or 0) + interval, 0.3)
    c.near("the client's stack drops one interval after it stepped in", decay_c[0].get("t") if decay_c else None, (step.get("t") or 0) + interval, 0.3)
    c.check("nobody burns any more at the end", (host.stats(23.5, AI) or {}).get("burning") == 0.0 and (host.stats(23.5, C) or {}).get("burning") == 0.0)
    total_burn = human["hp_max"] - bolt["damage"] - (val(host, 23.5, AI, "hp") or 0)
    c.near("total burning damage of one stack over its interval", total_burn, burn["damage_per_s_per_stack"] * interval, burn["damage_per_s_per_stack"] * 1.0)
    c.near("the client took the same from its stack", human["hp_max"] - (val(host, 23.5, C, "hp") or 0), burn["damage_per_s_per_stack"] * interval, burn["damage_per_s_per_stack"] * 1.0)
    c.near("the caster did not burn", val(host, 23.5, H, "hp"), slime["hp_max"], 0.01)

    print("--- fire bolt against a block")
    b4 = one("the fire bolt hits the blocking AI", host.find("HydraBolt", 24.5, 25.6, attacker=H, target=AI))
    c.check("it bursts on the shield", b4.get("result") == "blocked")
    c.near("only part of the damage gets through a plain shield", b4.get("dmg"), bolt["damage"] * v["block"]["projectile_damage_through"], 0.01)
    c.near("the blocker pays stamina", b4.get("target_stamina"), human["stamina_max"] - v["block"]["stamina_cost_per_hit"], 0.01)
    f2 = one("the fire field appears in front of the shield", host.find("HydraField", 24.5, 25.6))
    c.near("... between the two, not under the blocker", f2.get("y"), 600 - (human["body_radius_m"] * 100 + field["radius_m"] * 100), 5)
    c.check("the blocker does not burn", b4.get("burning") == 0.0 and (host.stats(26, AI) or {}).get("burning") == 0.0)

    print("--- casting respects the turn rate")
    cast5 = one("cast aimed 90 degrees to the side", host.find("HydraCast", 27.5, 28.2, attacker=H))
    attack_rate = max(slime_turn * tr["attacking_factor"], tr["min_deg_s"])
    possible = attack_rate * bolt["windup_ms"] / 1000.0
    c.check("the aim is further than the slime can turn during the wind-up", possible < 85, "can turn %.0f degrees" % possible)
    c.between("the bolt leaves along the facing reached at the turn rate (%.0f deg/s while casting)" % attack_rate, cast5.get("yaw"), possible - 12, possible + 12)
    c.check("... so it does not hit the AI it was aimed at", len(host.find("HydraBolt", 27.5, 29.5, attacker=H, target=AI)) == 0)
    cast6 = one("charged cast while the camera turns", host.find("HydraCast", 30.5, 31.6, attacker=H))
    c.check("the slime could follow the camera in that time", attack_rate * 0.6 > 40, "%.0f deg/s" % attack_rate)
    c.between("while it charges the caster follows the camera: the bolt leaves where the camera looks at the release", cast6.get("yaw"), 37, 43)
    c.check("... and was charged", cast6.get("charge", 0) > 0.5, cast6.get("charge"))

    print("--- screenshots")
    shots = os.path.join(root, "Saved", "Screenshots", "LiveSlime")
    for name in ("host_9.9.png", "host_12.4.png", "host_25.4.png", "client_5.8.png", "client_13.2.png"):
        path = os.path.join(shots, name)
        c.check("screenshot " + name, os.path.exists(path) and os.path.getsize(path) > 20000,
                "%d bytes" % os.path.getsize(path) if os.path.exists(path) else "missing")

    print("passed=%d failed=%d" % (c.passed, c.failed))
    return 0 if c.failed == 0 and c.passed > 0 else 1


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
