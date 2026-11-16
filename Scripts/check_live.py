"""Checks the logs and screenshots of a scripted live run (called by Scripts/run_live.sh).

  check_live.py <project root>            check Saved/Logs/LiveHost.log and LiveClient.log
  check_live.py --edit-values <file>      change the copied values.json for the reload step

The expected numbers are worked out here from data/values.json with the formulas of the
design (not taken from the game), so a wrong formula in the game shows up as a failed check.
Runs with the Python that ships with the engine; standard library only.
Exit code 0 = every check passed.
"""
import json
import math
import os
import re
import sys

# Values the reload step switches to.
STAMINA_MAX_FACTOR = 1.5
SLOW_TURN_BASE_DEG_S = 60.0
SLOW_TURN_MIN_DEG_S = 30.0


def edit_values(path):
    with open(path, encoding="utf-8") as f:
        values = json.load(f)
    values["forms"]["humanoid"]["stamina_max"] *= STAMINA_MAX_FACTOR
    values["turn_rate"]["min_deg_s"] = SLOW_TURN_MIN_DEG_S
    values["turn_rate"]["base_deg_s"] = SLOW_TURN_BASE_DEG_S
    with open(path, "w", encoding="utf-8") as f:
        json.dump(values, f, indent=2)
    print("check_live: edited " + path)


def read_log(path):
    if not os.path.exists(path):
        return []
    with open(path, encoding="utf-8", errors="replace") as f:
        return f.read().splitlines()


def parse_fields(text):
    fields = {}
    for key, value in re.findall(r"(\w+)=(\S+)", text):
        try:
            fields[key] = float(value)
        except ValueError:
            fields[key] = value
    return fields


def angle_diff(a, b):
    return abs((a - b + 180.0) % 360.0 - 180.0)


class Log:
    def __init__(self, path):
        self.lines = read_log(path)
        self.events = {}
        for line in self.lines:
            for kind in ("HydraStats", "HydraHit", "HydraMiss", "HydraDeath", "HydraRespawn", "HydraAimAssist", "HydraAutoAttack", "HydraCombatStats"):
                marker = kind + " "
                if marker in line:
                    fields = parse_fields(line[line.index(marker) + len(marker):])
                    for key in ("id", "attacker", "target", "who", "lock"):
                        if key in fields:
                            fields[key] = str(fields[key]).replace(".0", "") if isinstance(fields[key], float) else fields[key]
                    self.events.setdefault(kind, []).append(fields)
        own = [s for s in self.events.get("HydraStats", []) if s.get("me") == 1.0]
        self.own_id = own[0]["id"] if own else None

    def count(self, text):
        return sum(1 for line in self.lines if text in line)

    def stats(self, t, char_id):
        """The stats line for a character at dev clock t (the scripted calls are at least 0.3 s apart)."""
        for s in self.events.get("HydraStats", []):
            if s["id"] == char_id and abs(s["t"] - t) < 0.2:
                return s
        return None

    def find(self, kind, t0, t1, **match):
        """Events of a kind between dev clock t0 and t1 with the given field values."""
        result = []
        for e in self.events.get(kind, []):
            if t0 <= e.get("t", -1) <= t1 and all(e.get(k) == v for k, v in match.items()):
                result.append(e)
        return result


class Checker:
    def __init__(self):
        self.failed = 0
        self.passed = 0

    def check(self, name, ok, detail=""):
        if ok:
            self.passed += 1
        else:
            self.failed += 1
        print("%s  %s%s" % ("ok  " if ok else "FAIL", name, ("   [" + str(detail) + "]") if detail != "" else ""))

    def near(self, name, value, expected, tolerance):
        self.check(name, value is not None and expected is not None and abs(value - expected) <= tolerance,
                   "is %s, expected %s +- %.2f" % ("missing" if value is None else "%.2f" % value, "?" if expected is None else "%.2f" % expected, tolerance))

    def between(self, name, value, low, high):
        self.check(name, value is not None and low <= value <= high,
                   "is %s, expected %.1f .. %.1f" % ("missing" if value is None else "%.1f" % value, low, high))


def main(root):
    with open(os.path.join(root, "data", "values.json"), encoding="utf-8") as f:
        v = json.load(f)

    # --- The rules, worked out from the data (design formulas) -----------------------------
    form, sword, shield = v["forms"]["humanoid"], v["weapons"]["longsword"], v["shields"]["round_shield"]
    hp_max, st_max, regen = form["hp_max"], form["stamina_max"], form["stamina_regen_per_s"]
    weight = form["body_weight"] + sword["weight"] + shield["weight"]
    ratio = weight / form["strength"]
    weapon_ratio = sword["weight"] / form["strength"]
    light_cost = v["attack"]["light_stamina_per_weight_ratio"] * weapon_ratio
    heavy_cost = v["attack"]["heavy_stamina_per_weight_ratio"] * weapon_ratio
    dodge_cost = v["dodge"]["stamina_cost"] * ratio
    light, heavy = sword["light"], sword["heavy"]
    chain_bonus, chain_cap = v["chain"]["damage_bonus_per_hit"], v["chain"]["max_bonus"]
    block_ratio = min(max(sword["weight"] / shield["weight"], v["block"]["weight_ratio_min"]), v["block"]["weight_ratio_max"])
    block_cost_light = v["block"]["stamina_cost_per_hit"] * block_ratio
    block_cost_heavy = block_cost_light * v["block"]["heavy_attack_factor"]
    rebound_light = v["block"]["rebound_attacker_stamina_ratio"] * light_cost / block_ratio
    rebound_heavy = v["block"]["rebound_attacker_stamina_ratio"] * heavy_cost / block_ratio
    blocked = 1.0 - shield["block_damage_reduction"]
    weak = min(max(1.0 + (v["weak_point"]["damage_multiplier"] - 1.0) * v["weak_point"]["reference_weapon_weight_ratio"] / weapon_ratio, 1.0), v["weak_point"]["max_damage_multiplier"])
    interrupt_threshold = v["interrupt"]["damage_threshold_per_weight"] * (weight + v["interrupt"]["attacking_weapon_weight_factor"] * sword["weight"])
    full_charge = heavy["damage"] * heavy["full_charge_damage_factor"]
    tr = v["turn_rate"]

    def turn_rate(base, minimum, blocking=False, attacking=False):
        rate = min(base / (ratio ** tr["weight_strength_exponent"]), max(minimum, tr["max_deg_s"]))
        rate *= tr["blocking_factor"] if blocking else 1.0
        rate *= tr["attacking_factor"] if attacking else 1.0
        return max(rate, minimum)

    host = Log(os.path.join(root, "Saved", "Logs", "LiveHost.log"))
    client = Log(os.path.join(root, "Saved", "Logs", "LiveClient.log"))
    c = Checker()

    def val(log, t, char_id, key):
        s = log.stats(t, char_id)
        return s[key] if s else None

    def one(name, events):
        c.check(name, len(events) == 1, "%d found" % len(events))
        return events[0] if events else {}

    print("--- host mode: listen server, client window and the AI opponent")
    c.check("host: dev clock started", host.count("HydraDev: clock started") == 1)
    c.check("client: dev clock started", client.count("HydraDev: clock started") == 1)
    if host.own_id is None or client.own_id is None:
        c.check("both windows know their own character", False)
        print("passed=%d failed=%d" % (c.passed, c.failed))
        return 1
    H, C, AI = host.own_id, client.own_id, "ai"
    c.check("host and client are different players", H != C, "ids %s and %s" % (H, C))
    ids = sorted({s["id"] for s in host.events.get("HydraStats", [])})
    c.check("host sees host, client and the AI opponent", ids == sorted([H, C, AI]), ids)
    c.check("client sees the same three", sorted({s["id"] for s in client.events.get("HydraStats", [])}) == sorted([H, C, AI]))
    c.check("the AI opponent started passive (as asked on the command line)", host.count("HydraAI: opponent ready (passive)") == 1)
    c.check("no error lines from the game in either log", host.count("LogHydra: Error") + client.count("LogHydra: Error") == 0)
    c.check("no data warnings in either log", host.count("LogHydra: Warning: Data:") + client.count("LogHydra: Warning: Data:") == 0)

    print("--- weight and strength (numbers per form, weapon and shield from values.json)")
    cs = [e for e in host.events.get("HydraCombatStats", []) if e.get("ai") == 0.0]
    c.check("server logged the combat numbers of the players", len(cs) >= 2, "%d lines" % len(cs))
    first = cs[0] if cs else {}
    c.near("weight = body plus load", first.get("weight"), weight, 0.01)
    c.near("weight / strength", first.get("ratio"), ratio, 0.001)
    c.near("turn rate from weight / strength", first.get("turn"), turn_rate(tr["base_deg_s"], tr["min_deg_s"]), 0.2)
    c.near("light attack cost from weapon weight / strength", first.get("light_cost"), light_cost, 0.01)
    c.near("heavy attack cost", first.get("heavy_cost"), heavy_cost, 0.01)
    c.near("dodge cost from weight / strength", first.get("dodge_cost"), dodge_cost, 0.01)
    c.near("everyone starts with full health", val(host, 1, AI, "hp"), hp_max, 0.01)
    c.near("everyone starts with full stamina", val(host, 1, H, "stamina"), st_max, 0.01)
    c.near("teleported to the front of the AI (server)", val(host, 2, H, "y"), 440, 2)
    c.near("client sees the host there", val(client, 2, H, "y"), 440, 15)

    print("--- light attacks and the chain")
    h1 = one("first light attack hits", host.find("HydraHit", 2.5, 3.2, attacker=H, target=AI))
    h2 = one("second light attack hits", host.find("HydraHit", 3.3, 4.0, attacker=H, target=AI))
    h3 = one("third light attack hits", host.find("HydraHit", 4.1, 4.8, attacker=H, target=AI))
    c.check("they are light attacks, not blocked, not on a weak point", all(h.get("kind") == "light" and h.get("result") == "hit" and h.get("weak") == 0.0 for h in (h1, h2, h3)))
    c.near("hit lands after the wind-up", (h1.get("t") or 0) - 2.5, light["windup_ms"] / 1000.0, 0.15)
    c.near("first hit: weapon damage", h1.get("dmg"), light["damage"], 0.01)
    c.near("second hit of the chain deals more", h2.get("dmg"), light["damage"] * (1 + min(chain_bonus, chain_cap)), 0.01)
    c.near("third hit deals more again", h3.get("dmg"), light["damage"] * (1 + min(2 * chain_bonus, chain_cap)), 0.01)
    c.check("chain counts 1, 2, 3", [h1.get("chain"), h2.get("chain"), h3.get("chain")] == [1.0, 2.0, 3.0])
    c.near("attack cost taken from stamina (weight costs stamina)", h1.get("attacker_stamina"), st_max - light_cost, 0.01)
    total = light["damage"] * (3 + min(chain_bonus, chain_cap) + min(2 * chain_bonus, chain_cap))
    c.near("AI health after three hits (server)", val(host, 4.6, AI, "hp"), hp_max - total, 0.01)
    c.near("client sees the AI's health", val(client, 4.6, AI, "hp"), hp_max - total, 0.01)
    c.check("chain counter shown while the chain runs", val(host, 4.6, H, "chain") == 3.0)
    c.check("chain ends after %.0f s without a hit" % (v["chain"]["reset_after_ms"] / 1000.0), val(host, 7, H, "chain") == 0.0)
    h4 = one("fourth light attack after the pause hits", host.find("HydraHit", 7.2, 7.9, attacker=H, target=AI))
    c.near("... with plain weapon damage again", h4.get("dmg"), light["damage"], 0.01)
    c.check("... as the first hit of a new chain", h4.get("chain") == 1.0)

    print("--- client: attack, dodge and block are predicted locally and decided on the server")
    one("client's attack into the air is a miss on the server", host.find("HydraMiss", 5.0, 5.8, attacker=C))
    c.check("client shows its own attack at once (predicted)", (client.stats(5.4, C) or {}).get("action") in ("LightStrike", "Recovery", "LightWindup"), (client.stats(5.4, C) or {}).get("action"))
    c.near("client sees its attack cost (replicated)", val(client, 5.4, C, "stamina"), st_max - light_cost, 0.01)
    c.near("client dodged backwards by dodge.distance_m (server)", (val(host, 7, C, "x") or 0) - (val(host, 4.6, C, "x") or 0), v["dodge"]["distance_m"] * 100, 20)
    c.near("client and server agree on the position after the dodge", val(client, 7, C, "x"), val(host, 7, C, "x"), 15)
    c.check("client blocks: its own view", (client.stats(9.5, C) or {}).get("blocking") == 1.0)
    c.check("no stamina regeneration while blocking (client's view)", (client.stats(9.5, C) or {}).get("regenblocked") == 1.0)

    print("--- heavy attack: hold to charge")
    hv = one("heavy attack released early hits", host.find("HydraHit", 8.5, 9.3, attacker=H, target=AI))
    c.check("it is a heavy attack", hv.get("kind") == "heavy")
    c.near("released early: strikes at the minimum wind-up (the telegraph stays)", (hv.get("t") or 0) - 8.5, heavy["min_windup_ms"] / 1000.0, 0.15)
    c.near("released early: base heavy damage", hv.get("dmg"), heavy["damage"], 0.01)
    c.near("heavy attack costs the heavy stamina cost in total", hv.get("attacker_stamina"), st_max - heavy_cost, 0.01)
    hf = one("fully charged heavy attack hits", host.find("HydraHit", 10.5, 12.3, attacker=H, target=AI))
    c.near("strikes by itself at full charge", (hf.get("t") or 0) - 10.5, heavy["max_charge_ms"] / 1000.0, 0.15)
    c.near("full charge: more damage", hf.get("dmg"), full_charge, 0.01)
    c.check("the charge is visible to the other player (telegraph)", (client.stats(11, H) or {}).get("action") == "HeavyCharge", (client.stats(11, H) or {}).get("action"))
    c.check("the attacker shows it too", (host.stats(11, H) or {}).get("action") == "HeavyCharge")

    print("--- jump")
    rise = None if None in (val(host, 12.75, H, "z"), val(host, 11, H, "z")) else val(host, 12.75, H, "z") - val(host, 11, H, "z")
    jump_h = v["movement"]["jump_height_m"] * 100
    c.between("0.35 s after the jump button the character is in the air, below movement.jump_height_m", rise, jump_h * 0.5, jump_h + 3)
    seen = None if None in (val(client, 12.75, H, "z"), val(client, 11, H, "z")) else val(client, 12.75, H, "z") - val(client, 11, H, "z")
    c.between("the client sees the host in the air", seen, 15, jump_h + 3)

    print("--- weak point (back), death and respawn")
    wk = one("light attack on the back hits", host.find("HydraHit", 13.6, 14.4, attacker=H, target=AI))
    c.check("it is a weak-point hit", wk.get("weak") == 1.0)
    c.near("weak point: fixed multiplier, no random crit", wk.get("dmg"), light["damage"] * weak, 0.01)
    kill = one("heavy attack charged for 700 ms on the back hits", host.find("HydraHit", 14.5, 15.8, attacker=H, target=AI))
    charge = 1.0 + (heavy["full_charge_damage_factor"] - 1.0) * (700.0 - heavy["min_windup_ms"]) / (heavy["max_charge_ms"] - heavy["min_windup_ms"])
    # The charge is measured in frames, so allow one or two frames of charge either way.
    c.near("damage: partial charge x weak point x chain", kill.get("dmg"), heavy["damage"] * charge * weak * (1 + min(chain_bonus, chain_cap)), 2.5)
    c.check("guard rail: the worst single hit (full charge on the back) does not kill from full health", full_charge * weak < hp_max, "%.0f of %.0f" % (full_charge * weak, hp_max))
    death = one("the AI opponent is defeated", host.find("HydraDeath", 14.5, 17.0, who=AI))
    c.check("defeated on the server and on the client", (host.stats(17, AI) or {}).get("dead") == 1.0 and (client.stats(17, AI) or {}).get("dead") == 1.0)
    respawn = one("it respawns", host.find("HydraRespawn", 16.0, 19.8, who=AI))
    c.near("respawn after character.respawn_delay_ms", (respawn.get("t") or 0) - (death.get("t") or 0), v["character"]["respawn_delay_ms"] / 1000.0, 0.15)
    c.check("alive again with full health at its spawn point", (host.stats(19.8, AI) or {}).get("dead") == 0.0 and val(host, 19.8, AI, "hp") == hp_max and abs((val(host, 19.8, AI, "y") or 0) - 600) < 5)
    c.check("client sees it alive again", (client.stats(19.8, AI) or {}).get("dead") == 0.0)

    print("--- directional block, rebound, block break")
    c.check("AI holds its block (server and client see it)", (host.stats(21, AI) or {}).get("blocking") == 1.0 and (client.stats(21, AI) or {}).get("blocking") == 1.0)
    b1 = one("light attack from the front", host.find("HydraHit", 21.2, 21.9, attacker=H, target=AI))
    c.check("lands in the block", b1.get("result") == "blocked")
    c.near("blocked: strongly reduced damage", b1.get("dmg"), light["damage"] * blocked, 0.01)
    c.near("blocker pays stamina by the weight of the attack", b1.get("target_stamina"), st_max - block_cost_light, 0.01)
    c.near("rebound: the attacker loses extra stamina", b1.get("attacker_stamina"), st_max - light_cost - rebound_light, 0.01)
    b2 = one("heavy attack from the front", host.find("HydraHit", 22.2, 23.0, attacker=H, target=AI))
    c.check("lands in the block too", b2.get("result") == "blocked")
    c.near("heavy attack costs the blocker more", b2.get("target_stamina"), st_max - block_cost_light - block_cost_heavy, 0.01)
    c.near("no stamina regeneration while blocking", val(host, 23.2, AI, "stamina"), st_max - block_cost_light - block_cost_heavy, 0.01)
    c.check("a blocked hit does not advance the chain", b1.get("chain") == 0.0 and b2.get("chain") == 0.0)
    b3 = one("light attack from behind while the AI blocks", host.find("HydraHit", 24.0, 24.7, attacker=H, target=AI))
    c.check("the block does not cover the back: full hit on the weak point", b3.get("result") == "hit" and b3.get("weak") == 1.0)
    b4 = one("light attack into a block with too little stamina", host.find("HydraHit", 25.6, 26.3, attacker=H, target=AI))
    c.check("the block breaks", b4.get("result") == "blockbreak")
    c.near("the hit lands in full", b4.get("dmg"), light["damage"] * (1 + min(chain_bonus, chain_cap)), 0.01)
    c.near("the blocker's stamina is empty", b4.get("target_stamina"), 0, 0.01)

    print("--- interrupt by weight")
    i1 = one("light hit while the AI charges a heavy attack", host.find("HydraHit", 27.7, 28.4, attacker=H, target=AI))
    c.check("light damage is below the threshold for this weight: no interrupt", i1.get("interrupt") == 0.0 and light["damage"] < interrupt_threshold, "threshold %.1f" % interrupt_threshold)
    a1 = one("the AI's heavy attack plays out and hits the host", host.find("HydraHit", 28.4, 29.4, attacker=AI, target=H))
    c.near("same rules for the AI: full charge damage", a1.get("dmg"), full_charge, 0.01)
    i2 = one("heavy hit while the AI charges again", host.find("HydraHit", 30.2, 31.0, attacker=H, target=AI))
    c.check("heavy damage is above the threshold: interrupt", i2.get("interrupt") == 1.0 and heavy["damage"] >= interrupt_threshold)
    c.check("the interrupted attack never lands", len(host.find("HydraHit", 30.2, 32.4, attacker=AI)) + len(host.find("HydraMiss", 30.2, 32.4, attacker=AI)) == 0)
    c.check("no stun: the AI is simply back to no action", (host.stats(32, AI) or {}).get("action") == "None")
    c.near("the host was not hit", val(host, 32, H, "hp"), hp_max, 0.01)

    print("--- dodge: pure movement, no invulnerability")
    one("the AI attacks, the host steps out of the hit area: miss", host.find("HydraMiss", 33.0, 33.8, attacker=AI))
    c.check("... and no hit", len(host.find("HydraHit", 33.0, 33.8, attacker=AI)) == 0)
    c.near("the dodge moved the host back by dodge.distance_m", 440 - (val(host, 34, H, "y") or 0), v["dodge"]["distance_m"] * 100, 20)
    c.near("the host kept its health", val(host, 34, H, "hp"), hp_max, 0.01)

    print("--- lock-on and soft aim assist")
    s = host.stats(36, H)
    c.check("host locked on to the AI", s is not None and s["lock"] == AI, "" if s is None else s["lock"])
    c.between("camera points at the target", None if s is None else s["lockoff"], 0, 5)
    # The host was put 20 degrees off the AI (facing 70, AI at 90) before this attack.
    aa = one("aim assist found a target at the start of the attack",
             [e for e in host.events.get("HydraAimAssist", []) if e.get("attacker") == H and abs(e.get("yaw_before", 0) - 70) < 2])
    off = angle_diff(aa.get("yaw_before", 0), aa.get("yaw_to_target", 0))
    c.between("the target was off-centre", off, 10, v["aim_assist"]["angle_deg"])
    c.near("facing goal = part of the way to the target (aim_assist.strength)", angle_diff(aa.get("yaw_goal", 0), aa.get("yaw_before", 0)), off * v["aim_assist"]["strength"], 0.2)
    c.near("the character turned to the goal during the attack (server)", val(host, 38.3, H, "yaw"), aa.get("yaw_goal"), 1.5)
    c.near("client sees that facing", val(client, 38.3, H, "yaw"), aa.get("yaw_goal"), 3)

    print("--- network emulation and lag compensation (attacker with latency, target running away)")
    net = v["net_simulation"]
    lag = 150 // 2 if net["latency_is_round_trip"] else 150
    c.check("both sides applied latency step 150 ms", host.count("NetSim: step 150 ms (outgoing lag %d ms)" % lag) >= 1 and client.count("NetSim: step 150 ms (outgoing lag %d ms)" % lag) >= 1)
    c.check("client switched packet loss off through the server", host.count("NetSim: step 150 ms (outgoing lag %d ms), packet loss 0%% [server]" % lag) >= 1)
    c.between("measured ping of the client at step 150 ms", val(host, 43.5, C, "ping"), 130, 260)
    c.between("ping without emulation was low", val(host, 38.3, C, "ping"), 0, 70)
    fired = one("client's attack fired by what the client saw", [e for e in client.events.get("HydraAutoAttack", []) if "seen_dist" in e])
    lc = one("server: the client's attack hits the running host", host.find("HydraHit", 45.0, 46.8, attacker=C, target=H))
    reach = light["reach_m"] * 100 + 35
    c.between("server rewound by the attacker's ping", lc.get("rewind_ms"), 120, v["lag_compensation"]["max_rewind_ms"])
    c.between("rewound: the target was inside the hit area (what the attacker saw)", lc.get("dist"), 100, reach)
    c.check("at the server's present the target was already out of reach", lc.get("in_now") == 0.0 and (lc.get("dist_now") or 0) > reach, "dist_now %.0f, reach %.0f" % (lc.get("dist_now") or 0, reach))
    c.check("the hit landed on the back of the fleeing target (weak point)", lc.get("weak") == 1.0)
    lb = one("server: the client's second attack hits the host, who faces it", host.find("HydraHit", 47.9, 49.0, attacker=C, target=H))
    c.check("the host raised its block before the hit landed on the server", host.count("HydraAutoBlock pressed") == 1 and lb.get("block_now") == 1.0, "block_now=%s" % lb.get("block_now"))
    c.check("... but after what the attacker saw: the hit is not blocked (block state rewound)", lb.get("result") == "hit" and lb.get("weak") == 0.0, lb.get("result"))
    c.near("... and lands in full", lb.get("dmg"), light["damage"], 0.01)
    c.check("the client fired while it still saw the target close", fired.get("seen_dist", 999) < 170, fired.get("seen_dist"))
    c.check("emulation switched off again on both sides", host.count("NetSim: step 0 ms (outgoing lag 0 ms), packet loss 0% [server]") >= 1 and
            client.count("NetSim: step 0 ms (outgoing lag 0 ms), packet loss 0% [client]") >= 1)

    print("--- Hydra.ReloadValues (values edited on disk during the run) and the turn rate")
    c.check("host loaded the values twice (start and reload)", host.count("Loaded values.json v") == 2)
    c.check("client loaded the values twice (the server told it to reload)", client.count("Loaded values.json v") == 2)
    c.near("server: new stamina maximum", val(host, 51, H, "max"), st_max * STAMINA_MAX_FACTOR, 0.01)
    c.near("client sees the new maximum (replicated)", val(client, 51, C, "max"), st_max * STAMINA_MAX_FACTOR, 0.01)
    slow = turn_rate(SLOW_TURN_BASE_DEG_S, SLOW_TURN_MIN_DEG_S)
    slow_block = turn_rate(SLOW_TURN_BASE_DEG_S, SLOW_TURN_MIN_DEG_S, blocking=True)
    slow_attack = turn_rate(SLOW_TURN_BASE_DEG_S, SLOW_TURN_MIN_DEG_S, attacking=True)
    c.check("server logged the lowered turn rate", any(abs(e.get("turn", 0) - slow) < 0.2 for e in host.events.get("HydraCombatStats", [])), "expected %.1f" % slow)

    aa2 = [e for e in host.events.get("HydraAimAssist", []) if e.get("attacker") == H and abs(e.get("yaw_before", 0) - 62) < 2]
    aa2 = one("aim assist with the low turn rate", aa2)
    needed = angle_diff(aa2.get("yaw_goal", 0), aa2.get("yaw_before", 0))
    committed_s = (light["windup_ms"] + light["hit_ms"]) / 1000.0
    turned = None if val(host, 53, H, "yaw") is None else angle_diff(val(host, 53, H, "yaw"), aa2.get("yaw_before", 0))
    c.check("the goal is further than the turn rate allows during the attack", needed > slow_attack * committed_s + 1, "needs %.1f, allowed %.1f" % (needed, slow_attack * committed_s))
    c.between("aim assist turned only as far as the turn rate while attacking allows (%.0f deg/s)" % slow_attack, turned, slow_attack * committed_s * 0.6, slow_attack * committed_s + 2.5)

    y0, y1 = val(host, 53.8, H, "yaw"), val(host, 55.3, H, "yaw")
    free_turn = None if None in (y0, y1) else angle_diff(y1, y0)
    c.check("locked on with the camera on the target, character still turning", (host.stats(55.3, H) or {}).get("lock") == AI and (host.stats(55.3, H) or {}).get("lockoff", 99) < 8)
    c.between("1 s after locking on: turned about turn rate x 1 s (%.0f deg/s)" % slow, free_turn, slow * 0.7, slow * 1.3)
    y0, y1 = val(host, 56.8, H, "yaw"), val(host, 58.3, H, "yaw")
    block_turn = None if None in (y0, y1) else angle_diff(y1, y0)
    c.check("host is blocking during the second turn", (host.stats(58.3, H) or {}).get("blocking") == 1.0)
    c.between("while blocking: turned about the blocking turn rate x 1 s (%.0f deg/s)" % slow_block, block_turn, slow_block * 0.7, slow_block * 1.3)
    c.check("turning while blocking is clearly slower", free_turn is not None and block_turn is not None and block_turn < free_turn * 0.75, "%s vs %s" % (block_turn, free_turn))

    print("--- AI opponent, active")
    c.check("AI switched to active", host.count("HydraAI: active") == 1)
    acted = host.find("HydraHit", 59.2, 68, attacker=AI) + host.find("HydraMiss", 59.2, 68, attacker=AI)
    c.check("the AI attacked the player by itself", len(acted) >= 1, "%d attacks" % len(acted))
    # 3.8 s after it was switched on (it may have defeated the standing player later on and
    # walked off to the other one, so the end of the run says nothing about this).
    s_h2, s_ai2 = host.stats(63, H), host.stats(63, AI)
    if s_h2 and s_ai2 and s_h2.get("dead") == 0.0 and s_ai2.get("dead") == 0.0:
        d = ((s_h2["x"] - s_ai2["x"]) ** 2 + (s_h2["y"] - s_ai2["y"]) ** 2) ** 0.5
        c.between("the AI walked up to the player (it started 6 m away)", d, 60, light["reach_m"] * 100 + 60)
        to_player = math.degrees(math.atan2(s_h2["y"] - s_ai2["y"], s_h2["x"] - s_ai2["x"]))
        c.between("the AI faces the player (protects its back)", angle_diff(s_ai2["yaw"], to_player), 0, 25)
    else:
        c.check("the AI walked up to the player (someone was already defeated, position check skipped)", len(acted) >= 1)
    c.check("the AI pays stamina like a player", any(e.get("attacker_stamina", st_max * 9) < st_max * STAMINA_MAX_FACTOR for e in host.find("HydraHit", 59.2, 68, attacker=AI)) or len(host.find("HydraHit", 59.2, 68, attacker=AI)) == 0)

    print("--- screenshots")
    shots = os.path.join(root, "Saved", "Screenshots", "Live")
    for name in ("host_11.2.png", "host_17.2.png", "host_20.8.png", "host_36.1.png", "host_66.png", "client_11.3.png", "client_20.9.png", "client_66.1.png"):
        path = os.path.join(shots, name)
        c.check("screenshot " + name, os.path.exists(path) and os.path.getsize(path) > 20000,
                "%d bytes" % os.path.getsize(path) if os.path.exists(path) else "missing")

    print("passed=%d failed=%d" % (c.passed, c.failed))
    return 0 if c.failed == 0 and c.passed > 0 else 1


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--edit-values":
        edit_values(sys.argv[2])
        sys.exit(0)
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
