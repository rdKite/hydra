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
    # The run reads a copy of the data in which everyone starts at level 0 without skill levels
    # (phase-1.md section 26: bars from the attributes, costs from weight / body weight).
    slime_hp, slime_st, slime_mana = 10.0 * slime["vitality"], 10.0 * slime["endurance"], 10.0 * slime["spirit"]
    human_hp, human_st = 10.0 * human["vitality"], 10.0 * human["endurance"]
    mana_regen = slime_mana * slime["mana_regen_share_per_s"]
    slam_light_cost = v["attack"]["light_stamina_per_weight_ratio"] * slam["cost_ratio"] * slam["demand"]
    slam_heavy_cost = v["attack"]["heavy_stamina_per_weight_ratio"] * slam["cost_ratio"] * slam["demand"]
    slime_weight = slime["body_weight"] + slam["weight"]
    constant = v["resistance"]["curve_constant"]

    def resisted(damage, points):
        return damage * (1 - points / (points + constant)) if points >= 0 else damage * (1 + -points / constant)

    # Awakening gives fire magic level 1: its skill bonus is added to a fire bolt's damage.
    fire_bolt = bolt["damage"] * (1 + v["skills"]["damage_bonus_per_level"] * 1)
    blk, shield = v["block"], v["shields"]["round_shield"]
    blocker_share = min(max(blk["blocker_share"] * blk["reference_shield_ratio"] / (shield["weight"] / human["body_weight"]), blk["share_min"]), blk["share_max"])
    hold_per_s = blk["hold_stamina_per_s_per_weight_ratio"] * shield["weight"] / human["body_weight"]
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
    c.near("slime health: 10 per vitality", val(host, 1, H, "hp"), slime_hp, 0.01)
    c.near("slime: little stamina", val(host, 1, H, "stamina"), slime_st, 0.01)
    c.near("slime: a lot of mana", val(host, 1, H, "mana"), slime_mana, 0.01)
    c.near("the humanoid has no mana bar", val(host, 1, C, "maxmana"), 0, 0.01)
    c.near("client sees the slime's mana (replicated)", val(client, 1, H, "mana"), slime_mana, 0.01)
    c.near("the slime's body is low: its centre is half its height above the floor", (val(host, 1, C, "z") or 0) - (val(host, 1, H, "z") or 0),
           (human["body_height_m"] - slime["body_height_m"]) * 50, 3)
    cs = [e for e in host.events.get("HydraCombatStats", []) if e.get("form") == "slime"]
    c.near("slime turn rate from its weight / strength (capped)", cs[0].get("turn") if cs else None, slime_turn, 0.2)

    print("--- slam (melee, stamina)")
    s1 = one("light slam hits the AI", host.find("HydraHit", 2.5, 3.2, attacker=H, target=AI))
    c.near("slam damage (blunt, no resistance on the humanoid)", s1.get("dmg"), slam["light"]["damage"], 0.05)
    c.near("lands after the slam wind-up", (s1.get("t") or 0) - 2.5, slam["light"]["windup_ms"] / 1000.0, 0.15)
    c.near("slam costs stamina by its cost ratio (a body attack has no weight)", s1.get("attacker_stamina"), slime_st - slam_light_cost, 0.05)
    c.check("about two slams in a row for a slime at level 0", 2 * slam_light_cost <= slime_st < 3 * slam_light_cost, "cost %.0f of %.0f" % (slam_light_cost, slime_st))
    c.near("client sees the AI's health after the slam", val(client, 3.2, AI, "hp"), human_hp - slam["light"]["damage"], 0.05)
    s2 = one("charged slam hits", host.find("HydraHit", 4.0, 5.7, attacker=H, target=AI))
    c.check("it is the heavy slam", s2.get("kind") == "heavy")
    c.near("full charge damage", s2.get("dmg"), slam["heavy"]["damage"] * slam["heavy"]["full_charge_damage_factor"], 0.05)
    # First slam at 2.5 (0.8 s long), then 0.7 s of regeneration until the second press at 4.0.
    st_regen = slime_st * slime["stamina_regen_share_per_s"]
    c.near("the heavy slam costs its heavy cost in total; regeneration only between the two attacks", s2.get("attacker_stamina"),
           slime_st - slam_light_cost + st_regen * 0.7 - slam_heavy_cost, st_regen * 0.25)

    print("--- resistance: the slime shrugs off slash")
    r1 = one("the AI's sword hits the slime", host.find("HydraHit", 6.0, 6.8, attacker=AI, target=H))
    slashed = resisted(sword["light"]["damage"], slime["resistance"]["slash"])
    c.check("the slime has resistance points against slash", slime["resistance"]["slash"] > 0 and slashed < sword["light"]["damage"])
    c.near("slash damage is reduced by the resistance curve (points / (points + constant))", r1.get("dmg"), slashed, 0.05)
    c.near("slime health after the hit", r1.get("target_hp"), slime_hp - slashed, 0.05)

    print("--- bolt without an element (mana)")
    cast1 = one("cast by a tap", host.find("HydraCast", 7.5, 8.2, attacker=H))
    c.near("bolt leaves after the cast wind-up", (cast1.get("t") or 0) - 7.5, bolt["windup_ms"] / 1000.0, 0.15)
    c.check("no charge, no fire", cast1.get("charge") == 0.0 and cast1.get("fire") == 0.0)
    c.near("base mana cost paid at the press; no mana regeneration while the bolt is wound up", cast1.get("mana"), slime_mana - bolt["mana_cost"], 0.5)
    b1 = one("the bolt hits the AI", host.find("HydraBolt", 7.5, 8.3, attacker=H, target=AI))
    c.check("plain hit", b1.get("result") == "hit" and b1.get("fire") == 0.0)
    c.near("bolt damage", b1.get("dmg"), bolt["damage"], 0.05)
    c.check("no burning from a bolt without fire", b1.get("burning") == 0.0)
    c.check("no fire field from a bolt without fire", len(host.find("HydraField", 7.5, 8.5)) == 0)

    c.check("charging is shown (host and client)", (host.stats(9.8, H) or {}).get("action") == "CastCharge" and True)
    cast2 = one("fully charged cast", host.find("HydraCast", 9.0, 10.6, attacker=H))
    c.near("released by itself at full charge", (cast2.get("t") or 0) - 9.0, bolt["max_charge_ms"] / 1000.0, 0.15)
    c.near("full charge", cast2.get("charge"), 1.0, 0.001)
    b2 = one("the charged bolt hits from 6 m", host.find("HydraBolt", 10.0, 10.9, attacker=H, target=AI))
    c.near("charging makes it stronger", b2.get("dmg"), bolt["damage"] * bolt["full_charge_damage_factor"], 0.05)
    c.near("flight time for the distance at bolt.speed_m_s", (b2.get("t") or 0) - (cast2.get("t") or 0), (600 - 35 - 65) / (bolt["speed_m_s"] * 100.0), 0.08)
    mana_before = val(host, 9.8, H, "mana")
    mana_after = val(host, 10.8, H, "mana")
    extra = bolt["mana_cost"] * (bolt["full_charge_cost_factor"] - 1)
    # No regeneration while it charges (9.8 is inside the charge); from the release (10.2) to 10.8 it regenerates.
    c.near("... and costlier (extra mana at release, minus 0.6 s of regeneration after it)",
           None if None in (mana_before, mana_after) else mana_before - mana_after, extra - mana_regen * 0.6, mana_regen * 0.2)
    c.near("client sees the slime's mana go down", val(client, 10.8, H, "mana"), mana_after, mana_regen * 0.35)

    print("--- fire bolt, fire field, burning")
    cast3 = one("fire bolt cast", host.find("HydraCast", 11.5, 12.2, attacker=H))
    c.check("the bolt carries fire once fire is awakened", cast3.get("fire") == 1.0)
    b3 = one("the fire bolt hits the AI", host.find("HydraBolt", 11.5, 12.6, attacker=H, target=AI))
    c.near("fire damage: base x (1 + skill bonus of fire magic 1)", b3.get("dmg"), resisted(fire_bolt, human["resistance"]["fire"]), 0.05)
    c.check("the hit adds burning stacks", b3.get("burning") == float(burn["stacks_per_hit"]))
    f1 = one("a fire field appears where the bolt landed", host.find("HydraField", 11.5, 12.6))
    c.near("field under the target (x)", f1.get("x"), 0, 5)
    c.near("field under the target (y)", f1.get("y"), 600, 5)
    c.near("field radius", f1.get("radius"), field["radius_m"] * 100, 0.5)
    c.near("field duration", f1.get("duration"), field["duration_ms"] / 1000.0, 0.01)
    c.check("AI burns: server and client see the stack", (host.stats(14, AI) or {}).get("burning") == 1.0 and (client.stats(14, AI) or {}).get("burning") == 1.0)
    burned_at_14 = None if val(host, 14, AI, "hp") is None else human_hp - fire_bolt - val(host, 14, AI, "hp")
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
    total_burn = human_hp - fire_bolt - (val(host, 23.5, AI, "hp") or 0)
    c.near("total burning damage of one stack over its interval", total_burn, burn["damage_per_s_per_stack"] * interval, burn["damage_per_s_per_stack"] * 1.0)
    c.near("the client took the same from its stack", human_hp - (val(host, 23.5, C, "hp") or 0), burn["damage_per_s_per_stack"] * interval, burn["damage_per_s_per_stack"] * 1.0)
    c.near("the caster did not burn", val(host, 23.5, H, "hp"), slime_hp, 0.01)

    print("--- fire bolt against a block")
    b4 = one("the fire bolt hits the blocking AI", host.find("HydraBolt", 24.5, 25.6, attacker=H, target=AI))
    c.check("it bursts on the shield", b4.get("result") == "blocked")
    through = v["block"]["projectile_damage_through"]
    c.near("only part of the damage gets through a plain shield", b4.get("dmg"), fire_bolt * through, 0.05)
    held = hold_per_s * ((b4.get("t") or 25.0) - 24.0)
    c.near("the blocker pays a share of the damage the shield took (plus the holding cost since 24.0)", b4.get("target_stamina"),
           human_st - held - fire_bolt * (1 - through) * blocker_share, 12)
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
