"""Checks the logs and screenshots of a scripted live run (called by Scripts/run_live.sh).

  check_live.py <project root>            check Saved/Logs/LiveHost.log and LiveClient.log
  check_live.py --edit-values <file>      change the copied values.json for the reload step

Runs with the Python that ships with the engine; standard library only.
Exit code 0 = every check passed.
"""
import json
import os
import re
import sys

# Values the reload step switches to (factors on the real values, so the check follows the data).
STAMINA_MAX_FACTOR = 1.5
SLOW_TURN_RATE_DEG_S = 60.0


def edit_values(path):
    with open(path, encoding="utf-8") as f:
        values = json.load(f)
    values["stamina"]["max"] = values["stamina"]["max"] * STAMINA_MAX_FACTOR
    values["turn_rate"]["min_deg_s"] = SLOW_TURN_RATE_DEG_S / 2
    values["turn_rate"]["base_deg_s"] = SLOW_TURN_RATE_DEG_S
    with open(path, "w", encoding="utf-8") as f:
        json.dump(values, f, indent=2)
    print("check_live: edited " + path)


def read_log(path):
    if not os.path.exists(path):
        return []
    with open(path, encoding="utf-8", errors="replace") as f:
        return f.read().splitlines()


def parse_fields(line):
    return dict(re.findall(r"(\w+)=(\S+)", line))


def angle_diff(a, b):
    return abs((a - b + 180.0) % 360.0 - 180.0)


class Log:
    def __init__(self, path, who):
        self.lines = read_log(path)
        self.who = who
        self.stats = []
        for line in self.lines:
            if "HydraStats t=" in line:
                fields = parse_fields(line[line.index("HydraStats"):])
                for key in ("t", "stamina", "max", "x", "y", "yaw", "ctrl", "lockoff", "ping", "speed"):
                    fields[key] = float(fields[key])
                self.stats.append(fields)
        own = [s for s in self.stats if s["me"] == "1"]
        self.own_id = own[0]["id"] if own else None

    def count(self, text):
        return sum(1 for line in self.lines if text in line)

    def find(self, text):
        return [line for line in self.lines if text in line]

    def at(self, t, char_id):
        """The stats line for a character at dev clock t (the scripted calls are at least 0.3 s apart)."""
        for s in self.stats:
            if s["id"] == char_id and abs(s["t"] - t) < 0.25:
                return s
        return None


class Checker:
    def __init__(self):
        self.failed = 0
        self.passed = 0

    def check(self, name, ok, detail=""):
        if ok:
            self.passed += 1
        else:
            self.failed += 1
        print("%s  %s%s" % ("ok  " if ok else "FAIL", name, ("   [" + detail + "]") if detail else ""))

    def near(self, name, value, expected, tolerance):
        self.check(name, value is not None and abs(value - expected) <= tolerance,
                   "is %s, expected %.1f +- %.1f" % ("missing" if value is None else "%.1f" % value, expected, tolerance))

    def between(self, name, value, low, high):
        self.check(name, value is not None and low <= value <= high,
                   "is %s, expected %.1f .. %.1f" % ("missing" if value is None else "%.1f" % value, low, high))


def main(root):
    with open(os.path.join(root, "data", "values.json"), encoding="utf-8") as f:
        values = json.load(f)
    st = values["stamina"]
    smax, regen, delay = st["max"], st["regen_per_s"], st["regen_delay_after_action_ms"] / 1000.0
    new_max = smax * STAMINA_MAX_FACTOR
    run_speed = values["movement"]["run_speed_m_s"] * 100.0

    host = Log(os.path.join(root, "Saved", "Logs", "LiveHost.log"), "host")
    client = Log(os.path.join(root, "Saved", "Logs", "LiveClient.log"), "client")
    c = Checker()

    def val(log, t, char_id, key):
        s = log.at(t, char_id)
        return s[key] if s else None

    print("--- host mode: listen server plus a client window")
    c.check("host log exists and the dev clock started", host.count("HydraDev: clock started") == 1)
    c.check("client log exists and the dev clock started", client.count("HydraDev: clock started") == 1)
    c.check("host knows its own character", host.own_id is not None)
    c.check("client knows its own character", client.own_id is not None)
    if host.own_id is None or client.own_id is None:
        print("passed=%d failed=%d" % (c.passed, c.failed + 1))
        return 1
    H, C = host.own_id, client.own_id
    c.check("host and client are different players", H != C, "ids %s and %s" % (H, C))
    c.check("host sees three characters (host, client, dummy)", len({s["id"] for s in host.stats}) == 3, str(sorted({s["id"] for s in host.stats})))
    c.check("client sees three characters", len({s["id"] for s in client.stats}) == 3, str(sorted({s["id"] for s in client.stats})))
    c.check("both loaded the copied data folder", host.count("Saved/Live/data") + host.count("Saved\\Live\\data") > 0 and
            client.count("Saved/Live/data") + client.count("Saved\\Live\\data") > 0)
    c.check("no error lines from the game in either log", host.count("LogHydra: Error") + client.count("LogHydra: Error") == 0)

    print("--- stamina: cost, regeneration delay, regeneration (server-authoritative, replicated)")
    cost = 40.0
    c.near("host starts with a full bar", val(host, 1, H, "stamina"), smax, 0.1)
    c.near("client starts with a full bar (its own view)", val(client, 1, C, "stamina"), smax, 0.1)
    c.near("host: cost taken (0.3 s after spending)", val(host, 2.3, H, "stamina"), smax - cost, 0.1)
    c.check("host: regeneration blocked during the delay", (host.at(2.3, H) or {}).get("regenblocked") == "1")
    c.near("server: cost taken from the client's character", val(host, 2.3, C, "stamina"), smax - cost, 0.1)
    c.near("client sees its own cost (replicated)", val(client, 2.3, C, "stamina"), smax - cost, 0.1)
    # The two dev clocks start a moment apart, so the client may already see the host's first regeneration ticks.
    c.between("client sees the host's cost (replicated)", val(client, 2.3, H, "stamina"), smax - cost, smax - cost + regen * 0.3)
    expected = min(smax, smax - cost + regen * (1.0 - delay))
    c.near("host: regenerating 1 s after spending", val(host, 3, H, "stamina"), expected, regen * 0.25)
    c.check("host: delay over", (host.at(3, H) or {}).get("regenblocked") == "0")
    c.near("host: full again", val(host, 5, H, "stamina"), smax, 0.1)
    c.near("client: full again (its own view)", val(client, 5, C, "stamina"), smax, 0.1)
    c.near("dummy keeps a full bar", val(host, 5, "dummy", "stamina"), smax, 0.1)

    print("--- movement: automated input, server-authoritative, replicated")
    hx0, hy0 = val(host, 5, H, "x"), val(host, 5, H, "y")
    hx1, hy1 = val(host, 8, H, "x"), val(host, 8, H, "y")
    cx0, cx1 = val(host, 5, C, "x"), val(host, 8, C, "x")
    if None in (hx0, hy0, hx1, hy1, cx0, cx1):
        c.check("positions logged", False)
    else:
        moved = ((hx1 - hx0) ** 2 + (hy1 - hy0) ** 2) ** 0.5
        c.between("host moved about 1 s at run speed", moved, run_speed * 0.6, run_speed * 1.3)
        c.check("host moved forward and to the right", hx1 > hx0 + 100 and hy1 > hy0 + 200, "from (%.0f,%.0f) to (%.0f,%.0f)" % (hx0, hy0, hx1, hy1))
        c.between("server moved the client's character (client input)", cx1 - cx0, run_speed * 0.6, run_speed * 1.3)
        c.near("client sees the host where the server has it (x)", val(client, 8, H, "x"), hx1, 15)
        c.near("client sees the host where the server has it (y)", val(client, 8, H, "y"), hy1, 15)
        c.near("client and server agree on the client's position", val(client, 8, C, "x"), cx1, 15)
        c.near("host stands still again", val(host, 8, H, "speed"), 0, 1)

    print("--- soft aim assist")
    assist = host.find("HydraAimAssist target=")
    if len(assist) != 1:
        c.check("host ran the aim assist once", False, "%d lines" % len(assist))
    else:
        a = parse_fields(assist[0][assist[0].index("HydraAimAssist"):])
        before, to_target, after, strength = float(a["yaw_before"]), float(a["yaw_to_target"]), float(a["yaw_after"]), float(a["strength"])
        c.check("a target was inside the assist cone", a["target"] == "yes")
        c.near("strength comes from values.json", strength, values["aim_assist"]["strength"], 0.001)
        off = angle_diff(before, to_target)
        c.between("the target was off-centre (otherwise nothing to see)", off, 3, values["aim_assist"]["angle_deg"])
        c.near("turned by strength x the angle to the target", angle_diff(after, before), off * strength, 0.5)
        c.check("turned towards the target, not away", angle_diff(after, to_target) < off)
        c.near("server has the new facing", val(host, 9, H, "yaw"), after, 1)
        c.near("client sees the new facing", val(client, 9, H, "yaw"), after, 3)

    print("--- lock-on (turns only the camera)")
    for log, me, name in ((host, H, "host"), (client, C, "client")):
        s = log.at(11.5, me)
        c.check("%s: locked on to a target" % name, s is not None and s["lock"] != "none", "" if s is None else "lock=" + s["lock"])
        c.between("%s: camera points at the target" % name, None if s is None else s["lockoff"], 0, 5)
        c.near("%s: character faces where the camera looks" % name, None if s is None else angle_diff(s["yaw"], s["ctrl"]), 0, 5)
        s = log.at(13, me)
        c.check("%s: lock released" % name, s is not None and s["lock"] == "none")
    c.near("server has the client's facing while it is locked on (sent with the move)",
           None if None in (val(host, 11.5, C, "yaw"), val(client, 11.5, C, "yaw")) else angle_diff(val(host, 11.5, C, "yaw"), val(client, 11.5, C, "yaw")), 0, 5)

    print("--- network emulation (latency steps and packet loss from values.json)")
    net = values["net_simulation"]
    loss = round(net["packet_loss_ratio"] * 100)
    lag = 100 // 2 if net["latency_is_round_trip"] else 100
    c.check("host applied step 100 ms with packet loss", host.count("NetSim: step 100 ms (outgoing lag %d ms), packet loss %d%% [server]" % (lag, loss)) >= 1)
    c.check("client applied step 100 ms with packet loss", client.count("NetSim: step 100 ms (outgoing lag %d ms), packet loss %d%% [client]" % (lag, loss)) >= 1)
    c.check("client switched packet loss off through the server", host.count("NetSim: step 100 ms (outgoing lag %d ms), packet loss 0%% [server]" % lag) >= 1 and
            client.count("NetSim: step 100 ms (outgoing lag %d ms), packet loss 0%% [client]" % lag) >= 1)
    c.between("ping without emulation (client's view)", val(client, 13, C, "ping"), 0, 70)
    c.between("ping at step 100 ms (client's view)", val(client, 19, C, "ping"), 85, 220)
    c.between("ping at step 100 ms (host's view of the client)", val(host, 19, C, "ping"), 85, 220)
    c.check("emulation switched off again on both sides", host.count("NetSim: step 0 ms (outgoing lag 0 ms), packet loss 0% [server]") >= 1 and
            client.count("NetSim: step 0 ms (outgoing lag 0 ms), packet loss 0% [client]") >= 1)

    print("--- Hydra.ReloadValues (values edited on disk during the run)")
    c.check("host loaded the values twice (start and reload)", host.count("Loaded values.json v") == 2)
    c.check("client loaded the values twice (the server told it to reload)", client.count("Loaded values.json v") == 2)
    c.near("server: new stamina maximum", val(host, 24, H, "max"), new_max, 0.1)
    c.near("server: new maximum for the client's character", val(host, 24, C, "max"), new_max, 0.1)
    c.near("client sees the new maximum (replicated)", val(client, 24, C, "max"), new_max, 0.1)
    c.near("bar fills up to the new maximum", val(host, 27.5, H, "stamina"), new_max, 0.1)
    c.near("client: bar fills up to the new maximum", val(client, 27.5, C, "stamina"), new_max, 0.1)

    print("--- turn rate limits the character, not the camera (lock-on with the lowered turn rate)")
    s = host.at(25.5, H)
    c.check("host locked on to the dummy", s is not None and s["lock"] == "dummy", "" if s is None else "lock=" + s["lock"])
    c.between("0.5 s after locking: camera is on the target", None if s is None else s["lockoff"], 0, 8)
    behind = None if s is None else angle_diff(s["yaw"], s["ctrl"])
    before_lock = val(host, 24, H, "yaw")
    turned = None if s is None or before_lock is None else angle_diff(s["yaw"], before_lock)
    c.between("0.5 s after locking: character still far from the camera direction", behind, 12, 180)
    c.between("0.5 s after locking: character turned no faster than the turn rate (%.0f deg/s)" % SLOW_TURN_RATE_DEG_S, turned, 10, SLOW_TURN_RATE_DEG_S * 0.75)
    s = host.at(27.5, H)
    c.near("2.5 s after locking: character has caught up", None if s is None else angle_diff(s["yaw"], s["ctrl"]), 0, 5)
    c.near("client sees the host's final facing", val(client, 27.5, H, "yaw"), None if s is None else s["yaw"], 5) if s else c.check("final facing logged", False)

    print("--- screenshots")
    shots = os.path.join(root, "Saved", "Screenshots", "Live")
    for name in ("host_11.6.png", "host_27.6.png", "client_11.7.png", "client_27.7.png"):
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
