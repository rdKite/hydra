"""Checks the logs, screenshots and the metrics file of the waves live run (called by
Scripts/run_live_waves.sh).

  check_live_waves.py <project root>     check Saved/Logs/WavesHost.log and WavesClient.log
  check_live_waves.py --edit <file>      change the copied values.json for a short, fixed run

Like check_live.py, the expected numbers are worked out here with the formulas of the design -
from the copy of values.json the run used (Saved/LiveWaves/data). Exit code 0 = every check
passed.
"""
import csv
import glob
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from check_live import Checker, Log, parse_fields  # noqa: E402


def edit_values(path):
    with open(path, encoding="utf-8") as f:
        v = json.load(f)
    # Two small waves; the first enemy of the second has an element body.
    v["waves"]["first_delay_ms"] = 2000
    v["waves"]["break_ms"] = 3000
    v["waves"]["list"] = [
        {"groups": [{"enemy": "grunt", "count": 1}]},
        {"groups": [{"enemy": "ember_slime", "count": 1}, {"enemy": "grunt", "count": 1}]},
    ]
    # Low thresholds: one enemy is a level, two hits a skill level, two contacts an imprint.
    v["progression"]["xp_base"] = 20
    v["progression"]["xp_per_level"] = 1
    v["skills"]["xp_base"] = 1
    v["skills"]["xp_per_level"] = 0.4
    v["imprint"]["progress_first_level"] = 8
    v["imprint"]["progress_per_further_level"] = 16
    # Weak humanoids, so two slams defeat a grunt.
    v["forms"]["humanoid"]["hp_max"] = 40
    with open(path, "w", encoding="utf-8") as f:
        json.dump(v, f, indent=2)
    print("check_live_waves: edited " + path)


class WaveLog(Log):
    KINDS = ("HydraXp", "HydraLevelUp", "HydraSkillUp", "HydraSkillXp", "HydraImprint", "HydraEvolve", "HydraWave", "HydraWaveCleared",
             "HydraDodged", "HydraElementBody", "HydraChoose", "HydraWaves", "HydraMetrics")

    def __init__(self, path):
        super().__init__(path)
        for line in self.lines:
            for kind in self.KINDS:
                marker = kind + " "
                if marker in line and "exec '" not in line and "HydraDevCommand" not in line:
                    fields = parse_fields(line[line.index(marker) + len(marker):])
                    for key in ("who", "attacker", "body"):
                        if isinstance(fields.get(key), float):
                            fields[key] = str(int(fields[key]))
                    self.events.setdefault(kind, []).append(fields)
        # Ids in HydraCombatStats lines.
        for e in self.events.get("HydraCombatStats", []):
            if isinstance(e.get("who"), float):
                e["who"] = str(int(e["who"]))


def skills_of(stats):
    """skills=slam:20:0.0,dodge:20:8.0 -> {"slam": (20, 0.0), ...}"""
    result = {}
    text = (stats or {}).get("skills")
    if isinstance(text, str) and text != "none":
        for part in text.split(","):
            name, level, xp = part.split(":")
            result[name] = (int(level), float(xp))
    return result


def main(root):
    with open(os.path.join(root, "Saved", "LiveWaves", "data", "values.json"), encoding="utf-8") as f:
        v = json.load(f)
    P, S, I, IN = v["progression"], v["skills"], v["imprint"], v["instinct"]
    slime, human, elemental = v["forms"]["slime"], v["forms"]["humanoid"], v["forms"]["fire_elemental_slime"]
    slam, sword = v["weapons"]["slime_slam"], v["weapons"]["longsword"]
    grunt, ember = v["enemies"]["grunt"], v["enemies"]["ember_slime"]

    def xp_need(level):
        return P["xp_base"] + P["xp_per_level"] * level

    def skill_need(level):
        return S["xp_base"] + S["xp_per_level"] * level

    def level_up(level, xp, gained, cap):
        xp += gained
        while level < cap and xp >= xp_need(level):
            xp -= xp_need(level)
            level += 1
        return level, (0.0 if level >= cap else xp)

    def nxp(enemy):
        return enemy["nxp_base"] * (1 + P["nxp_bonus_per_enemy_level"] * enemy["level"])

    def damage_factor(level, skill):
        return (1 + P["damage_bonus_per_level"] * level) * (1 + S["damage_bonus_per_level"] * skill)

    def cost_factor(skill):
        return max(S["min_cost_factor"], 1 - S["cost_reduction_per_level"] * skill)

    host = WaveLog(os.path.join(root, "Saved", "Logs", "WavesHost.log"))
    client = WaveLog(os.path.join(root, "Saved", "Logs", "WavesClient.log"))
    c = Checker()

    def one(name, events):
        c.check(name, len(events) == 1, "%d found" % len(events))
        return events[0] if events else {}

    print("--- wave mode, start at fixed levels")
    c.check("host: dev clock started", host.count("HydraDev: clock started") == 1)
    if host.own_id is None or client.own_id is None:
        c.check("both windows know their own character", False)
        print("passed=%d failed=%d" % (c.passed, c.failed))
        return 1
    H, C = host.own_id, client.own_id
    c.check("no error lines from the game in either log", host.count("LogHydra: Error") + client.count("LogHydra: Error") == 0)
    c.check("no data warnings in either log", host.count("LogHydra: Warning: Data:") + client.count("LogHydra: Warning: Data:") == 0)
    one("waves started", host.events.get("HydraWaves", []))
    c.check("no practice opponent in wave mode", all(s["id"] != "ai" for s in host.events.get("HydraStats", [])))
    w1 = one("wave 1 came", [e for e in host.events.get("HydraWave", []) if e.get("wave") == 1.0])
    c.check("wave 1 is what the data says: one grunt", w1.get("enemies") == 1.0 and w1.get("who") == "grunt1", w1.get("who"))
    h1, c1 = host.stats(1, H) or {}, host.stats(1, C) or {}
    c.check("host is a slime at its start level", h1.get("form") == "slime" and h1.get("level") == slime["start_level"], h1.get("level"))
    c.check("client is a humanoid at its start level", c1.get("form") == "humanoid" and c1.get("level") == human["start_level"], c1.get("level"))
    c.check("the slime's skills start at the fixed skill level", skills_of(h1) == {k: (slime["start_skill_level"], 0.0) for k in ("slam", "projectile", "dodge")}, h1.get("skills"))
    c.check("the humanoid's skills too", skills_of(c1) == {k: (human["start_skill_level"], 0.0) for k in ("sword_fighting", "shield_fighting", "dodge")}, c1.get("skills"))
    c.near("level gives HP: slime", h1.get("maxhp"), slime["hp_max"] * (1 + P["hp_bonus_per_level"] * slime["start_level"]), 0.1)
    g1 = host.stats(1, "grunt1") or {}
    c.check("the grunt is a humanoid of its type's level", g1.get("form") == "humanoid" and g1.get("level") == grunt["level"], g1.get("level"))
    c.near("... with the HP of that level", g1.get("maxhp"), human["hp_max"] * (1 + P["hp_bonus_per_level"] * grunt["level"]), 0.1)
    c.check("the client sees levels and skills (replicated)", (client.stats(1, C) or {}).get("level") == human["start_level"]
            and skills_of(client.stats(1, C)) == skills_of(c1) and (client.stats(1, H) or {}).get("level") == slime["start_level"])
    c.check("nothing is offered yet", h1.get("offer") == "none" and h1.get("milestones") == 0.0)

    print("--- skill XP by use: a dodge counts only against a real attack")
    one("the grunt's attack misses the host", host.find("HydraMiss", 2.2, 3.2, attacker="grunt1"))
    one("the server saw that the dodge avoided a hit", [e for e in host.events.get("HydraDodged", []) if e.get("who") == H and e.get("attacker") == "grunt1"])
    dodge_step = v["dodge"]["distance_m"] * 100 * (1 + S["dodge_distance_bonus_per_level"] * slime["start_skill_level"])
    h3 = host.stats(3.2, H) or {}
    moved = ((h3.get("x", -1416) + 1416) ** 2 + (h3.get("y", -100) + 100) ** 2) ** 0.5
    c.near("dodge skill: the step is longer than the plain one", moved, dodge_step, 25)
    c.near("the host was not hit", h3.get("hp"), h1.get("maxhp"), 0.01)
    c.check("skill XP is not credited during the fight", skills_of(host.stats(3.2, H)).get("dodge") == (slime["start_skill_level"], 0.0))

    print("--- level and skill: more damage, lower cost")
    slam_skill = slime["start_skill_level"]
    light = slam["light"]["damage"] * damage_factor(slime["start_level"], slam_skill)
    hits = host.find("HydraHit", 4.0, 5.8, attacker=H, target="grunt1")
    c.check("two slams hit the grunt", len(hits) == 2, "%d" % len(hits))
    a, b = (hits + [{}, {}])[:2]
    c.near("slam damage = weapon x level factor x skill factor", a.get("dmg"), light, 0.02)
    c.near("second hit: chain on top", b.get("dmg"), light * (1 + min(v["chain"]["damage_bonus_per_hit"], v["chain"]["max_bonus"])), 0.02)
    cs = [e for e in host.events.get("HydraCombatStats", []) if e.get("who") == H and e.get("form") == "slime"]
    c.near("slam cost = weight / strength x skill factor", (cs[0] if cs else {}).get("light_cost"),
           v["attack"]["light_stamina_per_weight_ratio"] * slam["weight"] / slime["strength"] * cost_factor(slam_skill), 0.01)

    print("--- wave cleared: skill XP credited, nXP, level-up, skills rise")
    death = one("the grunt is defeated", [e for e in host.events.get("HydraDeath", []) if e.get("who") == "grunt1"])
    c.check("... by the host", str(death.get("killer")).replace(".0", "") == H, death.get("killer"))
    one("wave 1 cleared", [e for e in host.events.get("HydraWaveCleared", []) if e.get("wave") == 1.0])
    credited = {e.get("skill"): e for e in host.events.get("HydraSkillXp", []) if e.get("who") == H and "credited" in e and e.get("t", 99) < 9}
    c.near("slam: two hits credited", (credited.get("slam") or {}).get("credited"), 2 * S["xp_per_hit"], 0.01)
    c.near("dodge: one dodge credited", (credited.get("dodge") or {}).get("credited"), S["xp_per_dodge"], 0.01)
    c.check("the client did nothing: nothing credited", not [e for e in host.events.get("HydraSkillXp", []) if e.get("who") == C and e.get("t", 99) < 9])
    xp1 = [e for e in host.events.get("HydraXp", []) if e.get("t", 99) < 9]
    c.check("both players got the grunt's nXP (type and level of the enemy only)", sorted(e.get("who") for e in xp1) == sorted([H, C])
            and all(abs(e.get("nxp", 0) - nxp(grunt)) < 0.01 for e in xp1), [(e.get("who"), e.get("nxp")) for e in xp1])
    h_level, h_xp = level_up(slime["start_level"], 0.0, nxp(grunt), slime["level_cap"])
    c_level, c_xp = level_up(human["start_level"], 0.0, nxp(grunt), human["level_cap"])
    h6, c6 = host.stats(6.5, H) or {}, host.stats(6.5, C) or {}
    c.check("the slime reached its level cap", h_level == slime["level_cap"] and h6.get("level") == h_level, h6.get("level"))
    c.check("the humanoid levelled up as the XP curve says", c6.get("level") == c_level and c_level > human["start_level"], "%s, expected %d" % (c6.get("level"), c_level))
    c.near("... and keeps the rest of its XP", c6.get("xp"), c_xp, 0.05)
    c.near("the client sees its own XP (replicated)", (client.stats(6.5, C) or {}).get("xp"), c_xp, 0.05)
    # Skills rise at the level-up: one level per skill, with enough credited XP.
    exp_slam = (slam_skill + 1, 2 * S["xp_per_hit"] - skill_need(slam_skill)) if 2 * S["xp_per_hit"] >= skill_need(slam_skill) else (slam_skill, 2 * S["xp_per_hit"])
    exp_dodge = (slam_skill + 1, S["xp_per_dodge"] - skill_need(slam_skill)) if S["xp_per_dodge"] >= skill_need(slam_skill) else (slam_skill, S["xp_per_dodge"])
    got = skills_of(h6)
    c.check("slam rose by one level, the rest of its XP waits", exp_slam[0] == slam_skill + 1 and got.get("slam", (0, 0))[0] == exp_slam[0]
            and abs(got.get("slam", (0, 0))[1] - exp_slam[1]) < 0.05, "%s, expected %s" % (got.get("slam"), exp_slam))
    c.check("dodge: not enough XP for a level, it waits", exp_dodge[0] == slam_skill and got.get("dodge") is not None and got["dodge"][0] == exp_dodge[0]
            and abs(got["dodge"][1] - exp_dodge[1]) < 0.05, "%s, expected %s" % (got.get("dodge"), exp_dodge))
    c.check("projectile was not used: unchanged", got.get("projectile") == (slam_skill, 0.0), got.get("projectile"))
    c.check("the client sees the host's new skill level", skills_of(client.stats(6.5, H)).get("slam", (0, 0))[0] == exp_slam[0])
    c.check("milestones: the slime at its cap, the humanoid at level 10", h6.get("milestones") == 1.0 and c6.get("milestones") == 1.0, "%s / %s" % (h6.get("milestones"), c6.get("milestones")))
    c.check("... but nothing is offered without contact with fire", h6.get("offer") == "none" and c6.get("offer") == "none", "%s / %s" % (h6.get("offer"), c6.get("offer")))
    c.near("level-up: more maximum HP", h6.get("maxhp"), slime["hp_max"] * (1 + P["hp_bonus_per_level"] * h_level), 0.1)

    print("--- wave 2 and the element body: contact with fire builds the imprint")
    w2 = one("wave 2 came after the break", [e for e in host.events.get("HydraWave", []) if e.get("wave") == 2.0])
    cleared = [e for e in host.events.get("HydraWaveCleared", []) if e.get("wave") == 1.0]
    c.near("break between the waves", (w2.get("t") or 0) - ((cleared[0].get("t") if cleared else 0) or 0), v["waves"]["break_ms"] / 1000.0, 0.3)
    c.check("wave 2: an ember slime and a grunt", w2.get("who") == "ember_slime2,grunt3", w2.get("who"))
    e9 = host.stats(9.5, "ember_slime2") or {}
    c.check("the ember slime is a fire elemental slime with fire awakened", e9.get("form") == "fire_elemental_slime" and e9.get("fire") == 1.0 and e9.get("imprint") == 1.0)
    c.check("the client sees it", (client.stats(9.5, "ember_slime2") or {}).get("form") == "fire_elemental_slime")
    body = elemental["element_body_damage"] * (1 + P["damage_bonus_per_level"] * ember["level"])
    back_c = [e for e in host.events.get("HydraElementBody", []) if e.get("attacker") == C]
    c.check("the client's two sword hits get fire damage back", len(back_c) == 2, "%d" % len(back_c))
    c.near("element body damage (by the level of the body)", (back_c[0] if back_c else {}).get("dmg"), body, 0.02)
    sword_hits = host.find("HydraHit", 10.6, 12.4, attacker=C, target="ember_slime2")
    c.near("the slime line shrugs off slash: client's sword hit", (sword_hits[0] if sword_hits else {}).get("dmg"),
           sword["light"]["damage"] * damage_factor(c_level, human["start_skill_level"]) * (1 - elemental["resistance"]["slash"]), 0.03)
    c12 = client.stats(12.2, C) or {}   # the client's own view: all of this is replicated
    c.near("fire damage taken is imprint progress (kept up to what the level needs)", c12.get("imprintprogress"), min(I["progress_first_level"], 2 * body * I["progress_per_damage_taken"]), 0.05)
    c.check("the imprint is earned and the open milestone offers it", c12.get("offer") == "imprint", c12.get("offer"))
    c.check("the client's choice reached the server and was taken", any("HydraChoose who=%s choice=imprint taken" % C in line for line in host.lines))
    c13 = client.stats(13, C) or {}
    c.check("fire imprint level 1, milestone used, progress starts again", c13.get("imprint") == 1.0 and c13.get("milestones") == 0.0 and c13.get("imprintprogress") == 0.0,
            "%s / %s / %s" % (c13.get("imprint"), c13.get("milestones"), c13.get("imprintprogress")))
    c.check("awakening: the element skill fire magic starts at level 1", skills_of(c13).get("fire_magic", (0, 0))[0] == 1, c13.get("skills"))
    c.check("the server has the same", (host.stats(15.8, C) or {}).get("imprint") == 1.0 and (host.stats(15.8, C) or {}).get("fire") == 1.0 and (host.stats(15.8, C) or {}).get("milestones") == 0.0)
    cs_c = [e for e in host.events.get("HydraCombatStats", []) if e.get("who") == C]
    c.check("server worked out the numbers again for the client after its level-up and imprint", len(cs_c) >= 3, "%d lines" % len(cs_c))

    print("--- evolution with instinct")
    slam_now = exp_slam[0]
    back_h = [e for e in host.events.get("HydraElementBody", []) if e.get("attacker") == H and e.get("t", 0) < 16]
    c.check("the host's two slams get fire damage back", len(back_h) == 2, "%d" % len(back_h))
    slam_ember = host.find("HydraHit", 14.0, 15.8, attacker=H, target="ember_slime2")
    c.near("the slime line is vulnerable to blunt: slam with the risen skill", (slam_ember[0] if slam_ember else {}).get("dmg"),
           slam["light"]["damage"] * damage_factor(h_level, slam_now) * (1 - elemental["resistance"]["blunt"]), 0.03)
    h15 = host.stats(15.8, H) or {}
    c.check("at the level cap with the imprint earned: evolution and imprint are offered", h15.get("offer") == "imprint+evolve:fire_elemental_slime", h15.get("offer"))
    ev = one("the host evolved", [e for e in host.events.get("HydraEvolve", []) if e.get("who") == H])
    c.check("from slime to fire elemental slime", ev.get("from") == "slime" and ev.get("to") == "fire_elemental_slime" and ev.get("old_level") == h_level)
    h16, hc16 = host.stats(16.6, H) or {}, client.stats(16.6, H) or {}
    c.check("new form, level 0", h16.get("form") == "fire_elemental_slime" and h16.get("level") == 0.0, "%s level %s" % (h16.get("form"), h16.get("level")))
    c.check("it keeps the fire imprint at level 1: fire awakened", h16.get("imprint") == 1.0 and h16.get("fire") == 1.0)
    sk = skills_of(h16)
    c.check("skills start again at 0", all(sk.get(k, (9, 0))[0] == 0 for k in ("slam", "projectile", "dodge")), h16.get("skills"))
    c.check("fire magic is awake at level 1", sk.get("fire_magic", (0, 0))[0] == 1, h16.get("skills"))
    c.check("the milestone is used", h16.get("milestones") == 0.0 and h16.get("offer") == "none", h16.get("offer"))
    c.near("base values of the new form: HP", h16.get("maxhp"), elemental["hp_max"], 0.1)
    c.near("... full bars", h16.get("hp"), elemental["hp_max"], 0.1)
    c.near("... mana", h16.get("maxmana"), elemental["mana_max"], 0.1)
    c.check("the client sees the new form", hc16.get("form") == "fire_elemental_slime" and hc16.get("level") == 0.0 and hc16.get("fire") == 1.0, hc16.get("form"))
    ratio = min(1.0, slam_now / float(slime["level_cap"]))
    c.check("instinct recorded: old skill level / old level cap", ("slam:%.2f" % ratio) in str(ev.get("instinct")), ev.get("instinct"))
    cs_e = [e for e in host.events.get("HydraCombatStats", []) if e.get("who") == H and e.get("form") == "fire_elemental_slime"]
    effective = ratio * IN["skill_level_bonus_max"]
    c.near("instinct: a small permanent bonus on the skill (lower slam cost at skill level 0)", (cs_e[0] if cs_e else {}).get("light_cost"),
           v["attack"]["light_stamina_per_weight_ratio"] * slam["weight"] / elemental["strength"] * cost_factor(effective), 0.01)
    c.check("the model of the line is used for the new form", host.count("Form fire_elemental_slime uses the model slime") >= 1 and client.count("Form fire_elemental_slime uses the model slime") >= 1)

    print("--- after the evolution")
    last = host.find("HydraHit", 17.0, 19.0, attacker=H, target="ember_slime2")
    c.near("slam at level 0 with the instinct bonus", (last[0] if last else {}).get("dmg"),
           slam["light"]["damage"] * damage_factor(0, effective) * (1 - elemental["resistance"]["blunt"]), 0.03)
    one("the ember slime is defeated", [e for e in host.events.get("HydraDeath", []) if e.get("who") == "ember_slime2"])
    xp2 = [e for e in host.events.get("HydraXp", []) if e.get("who") == H and e.get("t", 0) > 16]
    ember_nxp = nxp(ember)
    c.near("its nXP", (xp2[0] if xp2 else {}).get("nxp"), ember_nxp, 0.01)
    e_level, e_xp = level_up(0, 0.0, ember_nxp, elemental["level_cap"])
    h19 = host.stats(19, H) or {}
    c.check("the new form levels from 0", h19.get("level") == e_level and e_level >= 1, "%s, expected %d" % (h19.get("level"), e_level))
    c.check("wave 2 is not cleared while the grunt stands", not [e for e in host.events.get("HydraWaveCleared", []) if e.get("wave") == 2.0] or host.find("HydraDeath", 0, 99, who="grunt3"))
    c.check("the ember slime is gone from the arena after a moment", host.stats(26, "ember_slime2") is None and client.stats(26, "ember_slime2") is None)

    print("--- wave enemies, active: same rules, reaction time and error rate per type")
    c.check("enemies switched to active", host.count("HydraAI: active") >= 1)
    acted = host.find("HydraHit", 19.5, 28, attacker="grunt3") + host.find("HydraMiss", 19.5, 28, attacker="grunt3")
    g26 = host.stats(26, "grunt3")
    if g26 is not None and g26.get("dead") == 0.0:
        c.check("the grunt came for the players and attacked", len(acted) >= 1, "%d attacks" % len(acted))
    else:
        c.check("the grunt fought (it was defeated or attacked)", len(acted) >= 1 or bool([e for e in host.events.get("HydraDeath", []) if e.get("who") == "grunt3"]))

    print("--- metrics CSV")
    files = glob.glob(os.path.join(root, "Saved", "LiveWaves", "metrics", "metrics_*.csv"))
    c.check("one metrics file was written", len(files) == 1, "%d files" % len(files))
    rows = []
    if files:
        with open(files[0], encoding="utf-8", newline="") as f:
            rows = list(csv.DictReader(f))
    need = ["time_s", "event", "wave", "kind", "who", "form", "level", "hits", "hits_blocked", "blocks", "dodges", "dodges_avoided", "stamina_used", "mana_used", "damage_dealt", "damage_taken"]
    c.check("it has the columns for hits, blocked hits, dodges and stamina use", bool(rows) and all(k in rows[0] for k in need), list(rows[0].keys()) if rows else "no rows")
    r1 = {(r["kind"], r["who"]): r for r in rows if r["event"] == "wave_cleared" and r["wave"] == "1"}
    rh, rc, rg = r1.get(("player", H), {}), r1.get(("player", C), {}), r1.get(("enemy", "grunt"), {})
    c.check("wave 1: a row for each player and for the enemy type", bool(rh) and bool(rc) and bool(rg), sorted(r1.keys()))
    c.check("host: 2 attacks, 2 hits, 1 dodge that avoided a hit, 1 kill", (rh.get("attacks"), rh.get("hits"), rh.get("dodges"), rh.get("dodges_avoided"), rh.get("kills")) == ("2", "2", "1", "1", "1"),
            (rh.get("attacks"), rh.get("hits"), rh.get("dodges"), rh.get("dodges_avoided"), rh.get("kills")))
    dodge_cost = v["dodge"]["stamina_cost"] * (slime["body_weight"] + slam["weight"]) / slime["strength"] * cost_factor(slam_skill)
    slam_cost = v["attack"]["light_stamina_per_weight_ratio"] * slam["weight"] / slime["strength"] * cost_factor(slam_skill)
    c.near("host: stamina use = one dodge and two slams", float(rh.get("stamina_used") or 0), dodge_cost + 2 * slam_cost, 0.2)
    c.near("host: damage dealt (both hits in full)", float(rh.get("damage_dealt") or 0), light * (2 + min(v["chain"]["damage_bonus_per_hit"], v["chain"]["max_bonus"])), 0.2)
    c.check("client: nothing happened in wave 1", (rc.get("attacks"), rc.get("hits"), rc.get("hits_taken")) == ("0", "0", "0"))
    c.check("grunt: 1 attack, 1 miss, 2 hits taken, 1 death", (rg.get("count"), rg.get("attacks"), rg.get("misses"), rg.get("hits_taken"), rg.get("deaths")) == ("1", "1", "1", "2", "1"),
            (rg.get("count"), rg.get("attacks"), rg.get("misses"), rg.get("hits_taken"), rg.get("deaths")))
    end = [r for r in rows if r["event"] == "session_end"]
    c.check("the rest of the session was written at the end", any(r["kind"] == "player" and r["who"] == H and r["form"] == "fire_elemental_slime" for r in end), [(r["kind"], r["who"]) for r in end])
    c.check("... with the ember slime of wave 2", any(r["kind"] == "enemy" and r["who"] == "ember_slime" and r["deaths"] == "1" for r in end))

    print("--- screenshots")
    shots = os.path.join(root, "Saved", "Screenshots", "LiveWaves")
    for name in ("host_7.png", "host_15.9.png", "host_16.8.png", "host_25.5.png", "client_13.2.png", "client_16.9.png"):
        path = os.path.join(shots, name)
        c.check("screenshot " + name, os.path.exists(path) and os.path.getsize(path) > 20000,
                "%d bytes" % os.path.getsize(path) if os.path.exists(path) else "missing")

    print("passed=%d failed=%d" % (c.passed, c.failed))
    return 0 if c.failed == 0 and c.passed > 0 else 1


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--edit":
        edit_values(sys.argv[2])
        sys.exit(0)
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
