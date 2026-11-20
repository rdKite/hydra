"""Checks the logs and screenshots of the test-tools live run (called by Scripts/run_live_debug.sh).

  check_live_debug.py <project root>

The commands of the debug panel are driven on host and client; every result is checked as
the server has it (host log) and as the other side sees it. Expected numbers come from
data/values.json. Exit code 0 = every check passed.
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from check_live import Checker  # noqa: E402
from check_live_waves import WaveLog, skills_of  # noqa: E402


def main(root):
    with open(os.path.join(root, "data", "values.json"), encoding="utf-8") as f:
        v = json.load(f)
    slime, human, P = v["forms"]["slime"], v["forms"]["humanoid"], v["progression"]
    grunt, ember = v["enemies"]["grunt"], v["enemies"]["ember_slime"]

    def bar(attribute, level):
        return 10.0 * attribute * (1 + P["level_growth"] * level)

    host = WaveLog(os.path.join(root, "Saved", "Logs", "DebugHost.log"))
    client = WaveLog(os.path.join(root, "Saved", "Logs", "DebugClient.log"))
    c = Checker()
    if host.own_id is None or client.own_id is None:
        c.check("both windows know their own character", False)
        print("passed=%d failed=%d" % (c.passed, c.failed))
        return 1
    H, C = host.own_id, client.own_id

    def both(t, who):
        return host.stats(t, who) or {}, client.stats(t, who) or {}

    print("--- own character: form, level, skills, imprint, milestone (host)")
    c.check("no error lines from the game in either log", host.count("LogHydra: Error") + client.count("LogHydra: Error") == 0)
    # The first stats line of the host, whenever it came (the first frames of a window can stall).
    s1 = next((e for e in host.events.get("HydraStats", []) if e.get("id") == H and e.get("t", 9) < 1.5), {})
    c.check("the host starts as a humanoid", s1.get("form") == "humanoid" and s1.get("level") == human["start_level"])
    s2, k2 = both(2, H)
    c.check("HydraSetForm: the host is a slime at the form's start levels", s2.get("form") == "slime" and s2.get("level") == slime["start_level"]
            and skills_of(s2) == {k: (slime["start_skill_level"], 0.0) for k in ("slam", "projectile")}, "%s %s %s" % (s2.get("form"), s2.get("level"), s2.get("skills")))
    c.check("... and the client sees it", k2.get("form") == "slime" and k2.get("level") == slime["start_level"])
    s4, k4 = both(4, H)
    c.check("HydraSetLevel: level 12", s4.get("level") == 12.0, s4.get("level"))
    c.near("... attributes and bars follow at once", s4.get("maxhp"), bar(slime["vitality"], 12), 0.1)
    c.near("... stamina too", s4.get("max"), bar(slime["endurance"], 12), 0.1)
    sk = skills_of(s4)
    c.check("HydraSetSkill: slam 7 with a half-full bar", sk.get("slam") == (7, 0.5), sk.get("slam"))
    c.check("... a skill cannot be set above the level: projectile 40 becomes 12", sk.get("projectile") == (12, 0.0), sk.get("projectile"))
    c.check("HydraSetImprint: fire imprint 2, fire awakened, fire magic there", s4.get("imprint") == 2.0 and s4.get("fire") == 1.0 and sk.get("fire_magic", (0, 0))[0] >= 1, s4.get("skills"))
    c.check("HydraMilestone: one open milestone, and what it offers", s4.get("milestones") == 1.0 and s4.get("offer") == "evolve:fire_elemental_slime", "%s %s" % (s4.get("milestones"), s4.get("offer")))
    c.check("the client sees all of it (replicated)", k4.get("level") == 12.0 and skills_of(k4) == sk and k4.get("imprint") == 2.0 and k4.get("milestones") == 1.0, k4.get("skills"))
    cs = [e for e in host.events.get("HydraCombatStats", []) if e.get("who") == H and e.get("form") == "slime"]
    slam, S, D = v["weapons"]["slime_slam"], v["skills"], v["damage"]
    # The last line of the run: level 13 and slam 8 (after the panel).
    g13 = P["level_growth"] * 13
    expected = slam["light"]["damage"] * (1 + S["damage_bonus_per_level"] * 8 + slam["strength_share"] * D["strength_bonus_per_gain"] * g13 + (1 - slam["strength_share"]) * D["dexterity_bonus_per_gain"] * g13)
    c.near("the server worked the numbers out again: slam damage with the set level and skill", (cs[-1] if cs else {}).get("light_dmg"), expected, 0.05)

    print("--- own character on the client: the server applies its requests too")
    t4, u4 = both(4, C)
    c.check("HydraSetLevel on the client: level 20 on the server", t4.get("level") == 20.0, t4.get("level"))
    c.check("HydraSetSkill on the client: projectile 15 with a half-full bar", skills_of(t4).get("projectile") == (15, 0.5), skills_of(t4).get("projectile"))
    c.check("the client sees its own new state", u4.get("level") == 20.0 and skills_of(u4).get("projectile") == (15, 0.5), u4.get("skills"))
    c.near("the client's bars follow", u4.get("maxhp"), bar(slime["vitality"], 20), 0.1)

    print("--- enemies: remove, spawn with a level, passive")
    c.check("HydraClearEnemies removed the practice opponent", host.stats(4, "ai") is not None and host.stats(5.2, "ai") is None and client.stats(5.2, "ai") is None)
    g, gc = both(5.2, "grunt1")
    c.check("HydraSpawn: a grunt at the chosen level", g.get("form") == "humanoid" and g.get("level") == 8.0, "%s level %s" % (g.get("form"), g.get("level")))
    c.near("... with the HP of that level", g.get("maxhp"), bar(human["vitality"], 8), 0.1)
    h5 = host.stats(5.2, H) or {}
    dist = None if not g or not h5 else ((g["x"] - h5["x"]) ** 2 + (g["y"] - h5["y"]) ** 2) ** 0.5
    c.between("... a few metres in front of the host", dist, 300, 500)
    c.check("... passive", host.count("HydraSpawn t=") >= 1 and any("who=grunt1" in line and "passive=1" in line for line in host.lines if "HydraSpawn t=" in line))
    c.check("the client sees the spawned enemy", gc.get("level") == 8.0)
    e, ec = both(6.5, "ember_slime2")
    c.check("HydraSpawn from the client: an ember slime at level 3", e.get("form") == "fire_elemental_slime" and e.get("level") == 3.0 and ec.get("level") == 3.0, "%s %s" % (e.get("form"), e.get("level")))
    k6 = host.stats(6.5, C) or {}
    dist = None if not e or not k6 else ((e["x"] - k6["x"]) ** 2 + (e["y"] - k6["y"]) ** 2) ** 0.5
    c.between("... in front of the client's character", dist, 300, 500)
    c.check("debug enemies do not start or end a wave", not host.events.get("HydraWave") and not host.events.get("HydraWaveCleared"))

    print("--- reset: full bars")
    c.check("stamina was spent before the reset", host.count("HydraSpendStamina amount=300.0 spent") == 1)
    r = host.stats(5.7, H) or {}
    c.near("HydraReset: full stamina 0.1 s later (regeneration alone could not do that)", r.get("stamina"), r.get("max"), 0.5)
    c.near("... full health", r.get("hp"), r.get("maxhp"), 0.01)

    print("--- the panel: the same path as the commands")
    c.check("panel opened and closed", host.count("HydraPanel open") == 1 and host.count("HydraPanel closed") == 1)
    s8, k8 = both(8.6, H)
    c.check("panel: down, right = level + 1", s8.get("level") == 13.0, s8.get("level"))
    c.check("panel: down, right = the first skill + 1", skills_of(s8).get("slam") == (8, 0.5), skills_of(s8).get("slam"))
    c.check("the client sees what the panel did", k8.get("level") == 13.0 and skills_of(k8).get("slam") == (8, 0.5))
    attacks = host.find("HydraHit", 7.4, 8.5, attacker=H) + host.find("HydraMiss", 7.4, 8.5, attacker=H)
    c.check("while the panel is open the game input is paused: the attack did not start", len(attacks) == 0, "%d" % len(attacks))
    c.check("the other window was not paused (its commands went on)", client.stats(8.6, C) is not None and host.stats(8.6, C) is not None)

    print("--- waves on and off")
    c.check("HydraWaves 1 started wave mode, HydraWaves 0 stopped it", any("started" in line for line in host.lines if "HydraWaves t=" in line)
            and any("stopped" in line for line in host.lines if "HydraWaves t=" in line))
    c.check("no wave came after it was switched off", not host.events.get("HydraWave"))

    print("--- looks of the humanoid base model: body preset and leg length")
    b, bc = both(10.7, "grunt1")
    c.check("before: neutral body, legs as built", (host.stats(10, "grunt1") or {}).get("body") == 0.0 and (host.stats(10, "grunt1") or {}).get("legs") == 1.0)
    c.check("HydraBody male and HydraLegs 0.8 reach every character on the server", b.get("body") == 1.0 and b.get("legs") == 0.8
            and (host.stats(10.7, H) or {}).get("body") == 1.0, "%s %s" % (b.get("body"), b.get("legs")))
    c.check("... and the client sees them (replicated)", bc.get("body") == 1.0 and bc.get("legs") == 0.8, "%s %s" % (bc.get("body"), bc.get("legs")))
    if os.path.exists(os.path.join(root, "Content", "Characters", "humanoid", "SK_humanoid.uasset")):
        c.check("the humanoid is shown with the base model and its thirteen clips (host and client)",
                host.count("Form humanoid uses the model humanoid with 13 animations") >= 1 and client.count("Form humanoid uses the model humanoid with 13 animations") >= 1)

    print("--- no evolution in combat")
    c.check("passive enemies are no combat: the open milestone offers the evolution, nothing waits",
            s8.get("offer") == "evolve:fire_elemental_slime" and s8.get("enemy_alive") == 0.0 and s8.get("evolve_waits") == 0.0, "%s %s %s" % (s8.get("offer"), s8.get("enemy_alive"), s8.get("evolve_waits")))
    w, wc = both(11.5, H)
    c.check("an active enemy is alive: the evolution waits (server)", w.get("enemy_alive") == 1.0 and w.get("evolve_waits") == 1.0, "%s %s" % (w.get("enemy_alive"), w.get("evolve_waits")))
    c.check("... and the client knows it too (replicated, for the HUD)", wc.get("enemy_alive") == 1.0 and (client.stats(11.5, C) or {}).get("enemy_alive") == 1.0)
    c.check("the evolve key is refused while the enemy lives", host.count("HydraChoose who=%s choice=evolve refused" % H) == 1 and host.count("HydraEvolveWaits") == 1)
    w2 = host.stats(12, H) or {}
    c.check("... the form stays and the milestone stays open", w2.get("form") == "slime" and w2.get("milestones") == 1.0 and w2.get("offer") == "evolve:fire_elemental_slime", "%s %s" % (w2.get("form"), w2.get("milestones")))
    f, fc = both(12.6, H)
    c.check("the enemies are gone: nothing waits any more (server and client)", f.get("enemy_alive") == 0.0 and f.get("evolve_waits") == 0.0 and fc.get("enemy_alive") == 0.0)
    c.check("now the evolve key is taken", host.count("HydraChoose who=%s choice=evolve taken" % H) == 1)
    e2, e2c = both(13.2, H)
    c.check("the host evolved, the milestone is used", e2.get("form") == "fire_elemental_slime" and e2.get("milestones") == 0.0 and e2c.get("form") == "fire_elemental_slime", "%s %s" % (e2.get("form"), e2.get("milestones")))

    print("--- screenshots")
    shots = os.path.join(root, "Saved", "Screenshots", "LiveDebug")
    for name in ("host_7.2.png", "host_10.3.png", "client_6.8.png"):
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
