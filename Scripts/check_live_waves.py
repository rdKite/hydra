"""Checks the logs, screenshots and the metrics file of the waves live run (called by
Scripts/run_live_waves.sh).

  check_live_waves.py <project root>     check Saved/Logs/WavesHost.log and WavesClient.log
  check_live_waves.py --edit <file>      change the copied values.json for a short, fixed run

Like check_live.py, the expected numbers are worked out here with the formulas of the design
(phase-1.md sections 17 and 26) - from the copy of values.json the run used
(Saved/LiveWaves/data). Exit code 0 = every check passed.
"""
import csv
import glob
import json
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from check_live import Checker, Log, parse_fields  # noqa: E402


def edit_values(path):
    with open(path, encoding="utf-8") as f:
        v = json.load(f)
    # Two small waves; the first enemy of the second has an element body.
    v["waves"]["first_delay_ms"] = 100
    v["waves"]["break_ms"] = 3000
    v["waves"]["list"] = [
        {"groups": [{"enemy": "grunt", "count": 1}]},
        {"groups": [{"enemy": "ember_slime", "count": 1}, {"enemy": "grunt", "count": 1}]},
    ]
    # Low thresholds: one enemy is a level, two contacts with fire an imprint.
    v["progression"]["xp_base"] = 20
    v["progression"]["xp_per_level"] = 1
    v["imprint"]["progress_first_level"] = 80
    v["imprint"]["progress_per_further_level"] = 160
    # A grunt that counts as hard: hXP = 2 x nXP, so the level-up gives three skill points.
    v["enemies"]["grunt"]["hxp_factor"] = 2.0
    # Weak bodies, so a bolt and two slams defeat a grunt and four slams an ember slime.
    v["forms"]["humanoid"]["vitality"] = 36
    v["forms"]["fire_elemental_slime"]["vitality"] = 49
    with open(path, "w", encoding="utf-8") as f:
        json.dump(v, f, indent=2)
    print("check_live_waves: edited " + path)


class WaveLog(Log):
    KINDS = ("HydraXp", "HydraHxp", "HydraLevelUp", "HydraSkillUp", "HydraImprint", "HydraEvolve", "HydraWave", "HydraWaveCleared",
             "HydraDodged", "HydraElementBody", "HydraChoose", "HydraWaves", "HydraMetrics", "HydraBolt", "HydraCast", "HydraUseDropped")

    def __init__(self, path):
        super().__init__(path)
        for line in self.lines:
            for kind in self.KINDS:
                marker = kind + " "
                if marker in line and "exec '" not in line and "HydraDevCommand" not in line:
                    fields = parse_fields(line[line.index(marker) + len(marker):])
                    for key in ("who", "attacker", "body", "target"):
                        if isinstance(fields.get(key), float):
                            fields[key] = str(int(fields[key]))
                    self.events.setdefault(kind, []).append(fields)


def skills_of(stats):
    """skills=slam:20:0.000,projectile:20:0.913 -> {"slam": (20, 0.0), ...} (level, progress bar)"""
    result = {}
    text = (stats or {}).get("skills")
    if isinstance(text, str) and text != "none":
        for part in text.split(","):
            name, level, progress = part.split(":")
            result[name] = (int(level), float(progress))
    return result


def distribute(points, weights, max_level, levels, progress):
    """The rule for automatic skill points (as HydraCombatMath::DistributeSkillPoints)."""
    names = list(weights)
    remaining = points
    for _ in range(len(names) + 1):
        eligible = [n for n in names if weights[n] > 0 and levels[n] < max_level]
        total = sum(weights[n] for n in eligible)
        if remaining <= 1e-6 or total <= 0:
            break
        excess = 0.0
        for n in eligible:
            progress[n] += remaining * weights[n] / total
            while progress[n] >= 1 - 1e-6 and levels[n] < max_level:
                progress[n] = max(0.0, progress[n] - 1)
                levels[n] += 1
            if levels[n] >= max_level:
                excess += progress[n]
                progress[n] = 0.0
        remaining = excess
    total = sum(max(0.0, w) for w in weights.values())
    if remaining > 1e-6 and total > 0:
        for n in names:
            progress[n] += remaining * max(0.0, weights[n]) / total


def main(root):
    with open(os.path.join(root, "Saved", "LiveWaves", "data", "values.json"), encoding="utf-8") as f:
        v = json.load(f)
    P, S, I, IN, D = v["progression"], v["skills"], v["imprint"], v["instinct"], v["damage"]
    slime, human, elemental = v["forms"]["slime"], v["forms"]["humanoid"], v["forms"]["fire_elemental_slime"]
    slam, sword, bolt = v["weapons"]["slime_slam"], v["weapons"]["longsword"], v["bolt"]
    grunt, ember = v["enemies"]["grunt"], v["enemies"]["ember_slime"]
    chain2 = 1 + min(v["chain"]["damage_bonus_per_hit"], v["chain"]["max_bonus"])
    constant = v["resistance"]["curve_constant"]

    def gain(level):
        return P["level_growth"] * level

    def bar(attribute, level):
        return 10.0 * attribute * (1 + gain(level))

    def xp_need(level):
        return P["xp_base"] + P["xp_per_level"] * level

    def nxp(enemy):
        return enemy["nxp_base"] * (1 + P["nxp_bonus_per_enemy_level"] * enemy["level"])

    def hxp(enemy):
        return nxp(enemy) * max(1.0, enemy["hxp_factor"])

    def points_for(rating):
        return v["skill_points"]["base"] + v["skill_points"]["factor"] * math.log2(max(1.0, rating))

    def weapon_damage(weapon, level, skill):
        g = gain(level)
        share = weapon["strength_share"]
        bonus = S["damage_bonus_per_level"] * skill + share * D["strength_bonus_per_gain"] * g + (1 - share) * D["dexterity_bonus_per_gain"] * g
        return weapon["light"]["damage"] * (1 + bonus)

    def resisted(damage, points):
        return damage * (1 - points / (points + constant)) if points >= 0 else damage * (1 + -points / constant)

    def level_ups(level, xp, gained, cap):
        """Returns the new level, the XP left and how many level-ups there were."""
        xp += gained
        ups = 0
        while level < cap and xp >= xp_need(level):
            xp -= xp_need(level)
            level += 1
            ups += 1
        return level, (0.0 if level >= cap else xp), ups

    slam_cost = v["attack"]["light_stamina_per_weight_ratio"] * slam["cost_ratio"] * slam["demand"]
    sword_cost = v["attack"]["light_stamina_per_weight_ratio"] * sword["weight"] / human["body_weight"] * sword["demand"]

    host = WaveLog(os.path.join(root, "Saved", "Logs", "WavesHost.log"))
    client = WaveLog(os.path.join(root, "Saved", "Logs", "WavesClient.log"))
    c = Checker()

    def one(name, events):
        c.check(name, len(events) == 1, "%d found" % len(events))
        return events[0] if events else {}

    def skill_is(name, stats, skill, level, progress):
        got = skills_of(stats).get(skill)
        c.check(name, got is not None and got[0] == level and abs(got[1] - progress) < 0.01, "%s, expected (%d, %.3f)" % (got, level, progress))

    print("--- wave mode, start at fixed levels, attributes by level")
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
    L0, K0 = slime["start_level"], slime["start_skill_level"]
    c.check("host is a slime at its start level", h1.get("form") == "slime" and h1.get("level") == L0, h1.get("level"))
    c.check("client is a humanoid at its start level", c1.get("form") == "humanoid" and c1.get("level") == human["start_level"], c1.get("level"))
    c.check("the slime's skills start at the fixed skill level; dodge is a capability, not a skill", skills_of(h1) == {k: (K0, 0.0) for k in ("slam", "projectile")}, h1.get("skills"))
    c.check("the humanoid's skills too", skills_of(c1) == {k: (human["start_skill_level"], 0.0) for k in ("sword_fighting", "shield_fighting")}, c1.get("skills"))
    c.near("the level acts through the attributes: HP = 10 x vitality x (1 + 2 % x level)", h1.get("maxhp"), bar(slime["vitality"], L0), 0.1)
    c.near("... stamina from endurance", h1.get("max"), bar(slime["endurance"], L0), 0.1)
    c.near("... mana from spirit", h1.get("maxmana"), bar(slime["spirit"], L0), 0.1)
    g1 = host.stats(1, "grunt1") or {}
    c.check("the grunt is a humanoid of its type's level", g1.get("form") == "humanoid" and g1.get("level") == grunt["level"], g1.get("level"))
    c.near("... with the HP of that level", g1.get("maxhp"), bar(human["vitality"], grunt["level"]), 0.1)
    c.check("the client sees levels and skills (replicated)", (client.stats(1, C) or {}).get("level") == human["start_level"]
            and skills_of(client.stats(1, C)) == skills_of(c1) and (client.stats(1, H) or {}).get("level") == L0)
    c.check("nothing is offered yet", h1.get("offer") == "none" and h1.get("milestones") == 0.0)
    cs = [e for e in host.events.get("HydraCombatStats", []) if e.get("who") == H and e.get("form") == "slime"]
    cs0 = cs[0] if cs else {}
    c.near("slam cost: its cost ratio only - neither level nor skill lowers it", cs0.get("light_cost"), slam_cost, 0.05)
    slam_dmg = weapon_damage(slam, L0, K0)
    c.near("slam damage = base x (1 + skill bonus + strength / dexterity bonus)", cs0.get("light_dmg"), slam_dmg, 0.05)
    c.near("the weak-point factor grows with dexterity", cs0.get("weak"), min(v["weak_point"]["damage_multiplier"] + v["weak_point"]["multiplier_per_dexterity_gain"] * gain(L0), v["weak_point"]["max_damage_multiplier"]), 0.002)

    print("--- dodge: a capability - distance and speed by dexterity, counted only against a real attack")
    one("the grunt's attack misses the host", host.find("HydraMiss", 2.2, 3.2, attacker="grunt1"))
    one("the server saw that the dodge avoided a hit", [e for e in host.events.get("HydraDodged", []) if e.get("who") == H and e.get("attacker") == "grunt1"])
    dodge_step = v["dodge"]["distance_m"] * 100 * (1 + v["dodge"]["distance_per_dexterity_gain"] * gain(L0))
    h3 = host.stats(3.2, H) or {}
    moved = ((h3.get("x", -1416) + 1416) ** 2 + (h3.get("y", -100) + 100) ** 2) ** 0.5
    c.check("dexterity makes the step longer than the plain one", dodge_step > v["dodge"]["distance_m"] * 100 + 20)
    c.near("the step is as long as dexterity says", moved, dodge_step, 25)
    c.near("the host was not hit", h3.get("hp"), h1.get("maxhp"), 0.01)
    c.check("no skill grows from dodging", skills_of(h3) == skills_of(h1), h3.get("skills"))

    print("--- level and skill: more damage, the same cost")
    b0 = one("the bolt hits the grunt", [e for e in host.events.get("HydraBolt", []) if e.get("attacker") == H and e.get("target") == "grunt1"])
    bolt_dmg = bolt["damage"] * (1 + S["damage_bonus_per_level"] * K0)
    c.near("bolt damage = base x (1 + projectile skill bonus); no strength or dexterity", b0.get("dmg"), bolt_dmg, 0.05)
    hits = host.find("HydraHit", 4.3, 6.2, attacker=H, target="grunt1")
    c.check("two slams hit the grunt", len(hits) == 2, "%d" % len(hits))
    a, b = (hits + [{}, {}])[:2]
    c.near("slam damage", a.get("dmg"), slam_dmg, 0.05)
    c.near("second hit: chain on top", b.get("dmg"), slam_dmg * chain2, 0.05)

    print("--- an enemy is defeated: hXP by use, nXP, level-up, automatic skill points")
    death = one("the grunt is defeated", [e for e in host.events.get("HydraDeath", []) if e.get("who") == "grunt1"])
    c.check("... by the host", str(death.get("killer")).replace(".0", "") == H, death.get("killer"))
    one("wave 1 cleared", [e for e in host.events.get("HydraWaveCleared", []) if e.get("wave") == 1.0])
    xp1 = [e for e in host.events.get("HydraXp", []) if e.get("t", 99) < 9]
    c.check("both players got the grunt's nXP (type and level of the enemy only)", sorted(e.get("who") for e in xp1) == sorted([H, C])
            and all(abs(e.get("nxp", 0) - nxp(grunt)) < 0.01 for e in xp1), [(e.get("who"), e.get("nxp")) for e in xp1])
    c.check("... and its hXP = nXP x the factor of its type", all(abs(e.get("hxp", 0) - hxp(grunt)) < 0.01 for e in xp1) and hxp(grunt) > nxp(grunt), [(e.get("who"), e.get("hxp")) for e in xp1])
    # Use since the start: one bolt (mana) and two slams (stamina); the dodge counts for no skill.
    use = {"slam": 2 * slam_cost, "projectile": bolt["mana_cost"]}
    shares = {k: hxp(grunt) * u / sum(use.values()) for k, u in use.items()}
    got_hxp = {e.get("skill"): e for e in host.events.get("HydraHxp", []) if e.get("who") == H and e.get("t", 99) < 9}
    c.check("the host's hXP went to the two skills it used", sorted(got_hxp) == ["projectile", "slam"], sorted(got_hxp))
    c.near("slam: use = the stamina of two slams", (got_hxp.get("slam") or {}).get("use"), use["slam"], 0.2)
    c.near("projectile: use = the mana of one bolt", (got_hxp.get("projectile") or {}).get("use"), use["projectile"], 0.2)
    c.near("slam's share of the hXP, in proportion to the use", (got_hxp.get("slam") or {}).get("hxp"), shares["slam"], 0.05)
    c.near("projectile's share", (got_hxp.get("projectile") or {}).get("hxp"), shares["projectile"], 0.05)
    c.check("the client did nothing: its hXP went to no skill", not [e for e in host.events.get("HydraHxp", []) if e.get("who") == C and e.get("t", 99) < 9])

    h_level, h_xp, h_ups = level_ups(L0, 0.0, nxp(grunt), slime["level_cap"])
    c_level, c_xp, c_ups = level_ups(human["start_level"], 0.0, nxp(grunt), human["level_cap"])
    h6, c6 = host.stats(6.5, H) or {}, host.stats(6.5, C) or {}
    c.check("the slime reached its level cap", h_level == slime["level_cap"] and h6.get("level") == h_level, h6.get("level"))
    c.check("the humanoid levelled up as the XP curve says", c6.get("level") == c_level and c_level > human["start_level"], "%s, expected %d" % (c6.get("level"), c_level))
    c.near("... and keeps the rest of its XP", c6.get("xp"), c_xp, 0.05)
    c.near("the client sees its own XP (replicated)", (client.stats(6.5, C) or {}).get("xp"), c_xp, 0.05)
    rating = hxp(grunt) / nxp(grunt)
    pts = points_for(rating)
    up = [e for e in host.events.get("HydraLevelUp", []) if e.get("who") == H and e.get("t", 99) < 9]
    c.near("hardship rating of the level = hXP / nXP", (up[0] if up else {}).get("hardship"), rating, 0.002)
    c.near("skill points = base + factor x log2(hardship rating)", (up[0] if up else {}).get("skill_points"), pts, 0.002)
    c.check("a hard enemy gives more than one point", pts > 1.5, "%.2f" % pts)
    levels = {"slam": K0, "projectile": K0}
    progress = {"slam": 0.0, "projectile": 0.0}
    for i in range(h_ups):
        distribute(pts, shares, L0 + 1 + i, levels, progress)
    skill_is("slam got its share of the points: whole points are levels, the rest stays on its bar", h6, "slam", levels["slam"], progress["slam"])
    skill_is("projectile got the smaller share", h6, "projectile", levels["projectile"], progress["projectile"])
    c.check("... and both together got all the points", abs((levels["slam"] - K0 + progress["slam"]) + (levels["projectile"] - K0 + progress["projectile"]) - pts * h_ups) < 0.01)
    c.check("the client sees the host's skill levels and bars", skills_of(client.stats(6.5, H)) == skills_of(h6), (client.stats(6.5, H) or {}).get("skills"))
    c.check("the client used no skill: its points are not handed out", skills_of(c6) == skills_of(c1), c6.get("skills"))
    c.near("the counters of the level start again after the level-up", h6.get("hardship"), 1.0, 0.001)
    c.check("milestones: the slime at its cap, the humanoid at level 10", h6.get("milestones") == 1.0 and c6.get("milestones") == 1.0, "%s / %s" % (h6.get("milestones"), c6.get("milestones")))
    c.check("... but nothing is offered without contact with fire", h6.get("offer") == "none" and c6.get("offer") == "none", "%s / %s" % (h6.get("offer"), c6.get("offer")))
    c.near("level-up: the attributes grew - more maximum HP", h6.get("maxhp"), bar(slime["vitality"], h_level), 0.1)
    c.near("... and more stamina", h6.get("max"), bar(slime["endurance"], h_level), 0.1)

    print("--- wave 2 and the element body: contact with fire builds the imprint")
    w2 = one("wave 2 came after the break", [e for e in host.events.get("HydraWave", []) if e.get("wave") == 2.0])
    cleared = [e for e in host.events.get("HydraWaveCleared", []) if e.get("wave") == 1.0]
    c.near("break between the waves", (w2.get("t") or 0) - ((cleared[0].get("t") if cleared else 0) or 0), v["waves"]["break_ms"] / 1000.0, 0.3)
    c.check("wave 2: an ember slime and a grunt", w2.get("who") == "ember_slime2,grunt3", w2.get("who"))
    e9 = host.stats(9.5, "ember_slime2") or {}
    c.check("the ember slime is a fire elemental slime with fire awakened", e9.get("form") == "fire_elemental_slime" and e9.get("fire") == 1.0 and e9.get("imprint") == 1.0)
    c.check("the client sees it", (client.stats(9.5, "ember_slime2") or {}).get("form") == "fire_elemental_slime")
    body = elemental["element_body_damage"] * (1 + I["damage_bonus_per_level"] * 1)
    back_c = [e for e in host.events.get("HydraElementBody", []) if e.get("attacker") == C]
    c.check("the client's two sword hits get fire damage back", len(back_c) == 2, "%d" % len(back_c))
    c.near("element body damage = its damage x (1 + imprint bonus of the body), no level factor", (back_c[0] if back_c else {}).get("dmg"), resisted(body, human["resistance"]["fire"]), 0.05)
    sword_hits = host.find("HydraHit", 10.6, 12.4, attacker=C, target="ember_slime2")
    sword_dmg = weapon_damage(sword, c_level, human["start_skill_level"])
    c.near("the slime line shrugs off slash (resistance points through the curve): client's sword hit", (sword_hits[0] if sword_hits else {}).get("dmg"),
           resisted(sword_dmg, elemental["resistance"]["slash"]), 0.05)
    c12 = client.stats(12.2, C) or {}   # the client's own view: all of this is replicated
    c.near("fire damage taken is imprint progress (kept up to what the level needs)", c12.get("imprintprogress"),
           min(I["progress_first_level"], 2 * resisted(body, human["resistance"]["fire"]) * I["progress_per_damage_taken"]), 0.1)
    c.check("the imprint is earned and the open milestone offers it", c12.get("offer") == "imprint", c12.get("offer"))
    c.check("the client's choice reached the server and was taken", any("HydraChoose who=%s choice=imprint taken" % C in line for line in host.lines))
    c13 = client.stats(13, C) or {}
    c.check("fire imprint level 1, milestone used, progress starts again", c13.get("imprint") == 1.0 and c13.get("milestones") == 0.0 and c13.get("imprintprogress") == 0.0,
            "%s / %s / %s" % (c13.get("imprint"), c13.get("milestones"), c13.get("imprintprogress")))
    c.check("awakening: the element skill fire magic starts at level 1", skills_of(c13).get("fire_magic", (0, 0))[0] == 1, c13.get("skills"))
    c.check("the server has the same", (host.stats(15.8, C) or {}).get("imprint") == 1.0 and (host.stats(15.8, C) or {}).get("fire") == 1.0 and (host.stats(15.8, C) or {}).get("milestones") == 0.0)

    print("--- evolution with instinct")
    slam_now = levels["slam"]
    back_h = [e for e in host.events.get("HydraElementBody", []) if e.get("attacker") == H and e.get("t", 0) < 16]
    c.check("the host's two slams get fire damage back", len(back_h) == 2, "%d" % len(back_h))
    slam_ember = host.find("HydraHit", 14.0, 15.8, attacker=H, target="ember_slime2")
    c.near("the slime line is weak to blunt (negative points): slam with the grown level and skill", (slam_ember[0] if slam_ember else {}).get("dmg"),
           resisted(weapon_damage(slam, h_level, slam_now), elemental["resistance"]["blunt"]), 0.05)
    h15 = host.stats(15.8, H) or {}
    c.check("at the level cap with the imprint earned: evolution and imprint are offered", h15.get("offer") == "imprint+evolve:fire_elemental_slime", h15.get("offer"))
    ev = one("the host evolved", [e for e in host.events.get("HydraEvolve", []) if e.get("who") == H])
    c.check("from slime to fire elemental slime", ev.get("from") == "slime" and ev.get("to") == "fire_elemental_slime" and ev.get("old_level") == h_level)
    h16, hc16 = host.stats(16.6, H) or {}, client.stats(16.6, H) or {}
    c.check("new form, level 0", h16.get("form") == "fire_elemental_slime" and h16.get("level") == 0.0, "%s level %s" % (h16.get("form"), h16.get("level")))
    c.check("it keeps the fire imprint at level 1: fire awakened", h16.get("imprint") == 1.0 and h16.get("fire") == 1.0)
    sk = skills_of(h16)
    c.check("skills start again at 0 with empty bars", all(sk.get(k) == (0, 0.0) for k in ("slam", "projectile")), h16.get("skills"))
    c.check("fire magic is awake at level 1", sk.get("fire_magic", (0, 0))[0] == 1, h16.get("skills"))
    c.check("the milestone is used", h16.get("milestones") == 0.0 and h16.get("offer") == "none", h16.get("offer"))
    c.near("base attributes of the new form at level 0: HP", h16.get("maxhp"), bar(elemental["vitality"], 0), 0.1)
    c.near("... full bars", h16.get("hp"), bar(elemental["vitality"], 0), 0.1)
    c.near("... mana", h16.get("maxmana"), bar(elemental["spirit"], 0), 0.1)
    c.check("the new form is weaker at first (less HP than the slime at its cap)", bar(elemental["vitality"], 0) < bar(slime["vitality"], h_level))
    c.check("the client sees the new form", hc16.get("form") == "fire_elemental_slime" and hc16.get("level") == 0.0 and hc16.get("fire") == 1.0, hc16.get("form"))
    ratio = min(1.0, slam_now / float(slime["level_cap"]))
    c.check("instinct recorded: old skill level / old level cap", ("slam:%.2f" % ratio) in str(ev.get("instinct")), ev.get("instinct"))
    cs_e = [e for e in host.events.get("HydraCombatStats", []) if e.get("who") == H and e.get("form") == "fire_elemental_slime"]
    effective = ratio * IN["skill_level_bonus_max"]
    evolved_slam = weapon_damage(slam, 0, effective)
    c.near("instinct: a small permanent bonus on the skill (slam damage at skill level 0)", (cs_e[0] if cs_e else {}).get("light_dmg"), evolved_slam, 0.05)
    c.near("the cost stays what the body attack costs", (cs_e[0] if cs_e else {}).get("light_cost"), slam_cost, 0.05)
    c.check("the model of the line is used for the new form", host.count("Form fire_elemental_slime uses the model slime") >= 1 and client.count("Form fire_elemental_slime uses the model slime") >= 1)

    print("--- after the evolution")
    last = host.find("HydraHit", 17.0, 19.0, attacker=H, target="ember_slime2")
    c.near("slam at level 0 with the instinct bonus", (last[0] if last else {}).get("dmg"), resisted(evolved_slam, elemental["resistance"]["blunt"]), 0.05)
    back_e = [e for e in host.events.get("HydraElementBody", []) if e.get("attacker") == H and e.get("t", 0) > 16.5]
    fire_points = elemental["resistance"]["fire"] + I["resistance_points_per_level"] * 1
    c.near("the fire elemental slime resists the element body's fire (form plus imprint points)", (back_e[0] if back_e else {}).get("dmg"), resisted(body, fire_points), 0.05)
    one("the ember slime is defeated", [e for e in host.events.get("HydraDeath", []) if e.get("who") == "ember_slime2"])
    xp2 = [e for e in host.events.get("HydraXp", []) if e.get("who") == H and e.get("t", 0) > 16]
    c.near("its nXP", (xp2[0] if xp2 else {}).get("nxp"), nxp(ember), 0.01)
    c.near("its hXP", (xp2[0] if xp2 else {}).get("hxp"), hxp(ember), 0.01)
    e_level, e_xp, e_ups = level_ups(0, 0.0, nxp(ember), elemental["level_cap"])
    h19 = host.stats(19, H) or {}
    c.check("the new form levels from 0", h19.get("level") == e_level and e_level >= 1, "%s, expected %d" % (h19.get("level"), e_level))
    # Only the slam was used after the evolution; it cannot rise above the level, the rest waits on its bar.
    e_levels, e_progress = {"slam": 0}, {"slam": 0.0}
    e_weight = {"slam": hxp(ember) * (1 + IN["xp_bonus_max"] * ratio)}
    e_pts = points_for(hxp(ember) / nxp(ember))
    for i in range(e_ups):
        distribute(e_pts, e_weight, 1 + i, e_levels, e_progress)
    skill_is("the slam is relearned: it rises with the level, never above it; what is left waits on its bar", h19, "slam", e_levels["slam"], e_progress["slam"])
    c.check("... the limit was the brake", e_levels["slam"] == e_level and e_progress["slam"] > 1.0, "%s %.2f" % (e_levels["slam"], e_progress["slam"]))
    # The client struck the ember slime twice with its sword since the grunt fell: its hXP go to sword fighting.
    c2_level, c2_xp, c2_ups = level_ups(c_level, c_xp, nxp(ember), human["level_cap"])
    s_levels, s_progress = {"sword_fighting": human["start_skill_level"]}, {"sword_fighting": 0.0}
    for i in range(c2_ups):
        distribute(e_pts, {"sword_fighting": hxp(ember)}, c_level + 1 + i, s_levels, s_progress)
    c19 = host.stats(19, C) or {}
    c.check("the client levelled up again", c19.get("level") == c2_level and c2_ups >= 1, "%s, expected %d" % (c19.get("level"), c2_level))
    skill_is("the client's sword fighting got all its points (the only skill it used)", c19, "sword_fighting", s_levels["sword_fighting"], s_progress["sword_fighting"])
    c.check("the client's shield skill was not used and did not move", skills_of(c19).get("shield_fighting") == (human["start_skill_level"], 0.0), c19.get("skills"))
    c.check("wave 2 is not cleared while the grunt stands", not [e for e in host.events.get("HydraWaveCleared", []) if e.get("wave") == 2.0] or host.find("HydraDeath", 0, 99, who="grunt3"))
    c.check("the ember slime is gone from the arena after a moment", host.stats(29, "ember_slime2") is None and client.stats(29, "ember_slime2") is None)

    print("--- wave enemies, active: same rules, reaction time and error rate per type")
    c.check("enemies switched to active", host.count("HydraAI: active") >= 1)
    acted = host.find("HydraHit", 19.5, 31, attacker="grunt3") + host.find("HydraMiss", 19.5, 31, attacker="grunt3")
    g29 = host.stats(29, "grunt3")
    if g29 is not None and g29.get("dead") == 0.0:
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
    got = (rh.get("attacks"), rh.get("hits"), rh.get("casts"), rh.get("bolt_hits"), rh.get("dodges"), rh.get("dodges_avoided"), rh.get("kills"))
    c.check("host: 2 attacks, 2 hits, 1 cast, 1 bolt hit, 1 dodge that avoided a hit, 1 kill", got == ("2", "2", "1", "1", "1", "1", "1"), got)
    dodge_cost = slime["dodge_stamina_cost"] * (slime["body_weight"] + slam["weight"]) / slime["body_weight"]
    c.near("host: stamina use = one dodge and two slams", float(rh.get("stamina_used") or 0), dodge_cost + 2 * slam_cost, 0.5)
    c.near("host: mana use = one bolt", float(rh.get("mana_used") or 0), bolt["mana_cost"], 0.5)
    c.near("host: damage dealt (all three hits in full)", float(rh.get("damage_dealt") or 0), bolt_dmg + slam_dmg * (1 + chain2), 0.5)
    c.check("client: nothing happened in wave 1", (rc.get("attacks"), rc.get("hits"), rc.get("hits_taken")) == ("0", "0", "0"))
    got = (rg.get("count"), rg.get("attacks"), rg.get("misses"), rg.get("hits_taken"), rg.get("deaths"))
    c.check("grunt: 1 attack, 1 miss, 3 hits taken, 1 death", got == ("1", "1", "1", "3", "1"), got)
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
