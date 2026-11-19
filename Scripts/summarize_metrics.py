"""Summarises the metrics CSV of a session (Saved/Metrics/metrics_<time>.csv).

  summarize_metrics.py [file or folder]     default: the newest file in Saved/Metrics

Run it through Scripts/summarize_metrics.sh (uses the Python that ships with the engine);
standard library only. Prints, per player and per enemy type, what happened in the session
and a few ratios that say something about the fight (how many attacks landed, how many
were blocked, how often a dodge avoided a hit, stamina per minute), then the rows per wave.
"""
import csv
import glob
import os
import sys

COUNTERS = ("attacks", "hits", "hits_blocked", "misses", "hits_taken", "blocks", "block_breaks", "dodges", "dodges_avoided",
            "casts", "bolt_hits", "damage_dealt", "damage_taken", "stamina_used", "mana_used", "deaths", "kills")


def newest(folder):
    files = sorted(glob.glob(os.path.join(folder, "metrics_*.csv")), key=os.path.getmtime)
    return files[-1] if files else None


def share(part, whole):
    return "-" if whole <= 0 else "%.0f %%" % (100.0 * part / whole)


def main(argv):
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    target = argv[1] if len(argv) > 1 else os.path.join(root, "Saved", "Metrics")
    path = newest(target) if os.path.isdir(target) else target
    if not path or not os.path.exists(path):
        print("No metrics file found in " + target + " (play a session with waves first: Scripts/play.sh)")
        return 1
    with open(path, encoding="utf-8", newline="") as f:
        rows = list(csv.DictReader(f))
    if not rows:
        print(path + " has no rows")
        return 1

    minutes = max(float(r["time_s"]) for r in rows) / 60.0
    waves = max(int(r["wave"]) for r in rows)
    print("Session: %s" % os.path.basename(path))
    print("  %.1f minutes, %d wave(s) reached, %d cleared" % (minutes, waves, len({r["wave"] for r in rows if r["event"] == "wave_cleared"})))

    totals = {}
    for r in rows:
        key = (r["kind"], r["who"])
        entry = totals.setdefault(key, {"form": r["form"], "level_first": r["level"], "level_last": r["level"], "count": 0})
        entry["form"], entry["level_last"] = r["form"], r["level"]
        entry["count"] += int(r["count"]) if r["kind"] == "enemy" else 0
        for c in COUNTERS:
            entry[c] = entry.get(c, 0.0) + float(r[c])

    for kind, title in (("player", "Players"), ("enemy", "Enemy types (all of a type together)")):
        print("\n" + title)
        for (k, who), t in sorted(totals.items()):
            if k != kind:
                continue
            label = "player %s" % who if kind == "player" else "%s x%d" % (who, t["count"])
            level = t["level_first"] if t["level_first"] == t["level_last"] else "%s -> %s" % (t["level_first"], t["level_last"])
            print("  %s (%s, level %s)" % (label, t["form"], level))
            landed = t["hits"] + t["hits_blocked"]
            # One attack can hit several characters, so hits are counted, not put against the attacks.
            print("    attacks %d (%d hit nobody): %d hits, of those blocked %s; bolts cast %d, hit %s"
                  % (t["attacks"], t["misses"], landed, share(t["hits_blocked"], landed), t["casts"], share(t["bolt_hits"], t["casts"])))
            incoming = t["hits_taken"] + t["blocks"]
            print("    hits against it %d: blocked %s, block breaks %d; dodges %d, of those avoided a hit %s"
                  % (incoming, share(t["blocks"], incoming), t["block_breaks"], t["dodges"], share(t["dodges_avoided"], t["dodges"])))
            print("    damage dealt %.0f, taken %.0f; stamina %.0f (%.0f per minute), mana %.0f; kills %d, deaths %d"
                  % (t["damage_dealt"], t["damage_taken"], t["stamina_used"], t["stamina_used"] / minutes if minutes > 0 else 0.0,
                     t["mana_used"], t["kills"], t["deaths"]))

    print("\nPer wave (players)")
    print("  %-6s %-13s %-8s %-6s %7s %5s %8s %7s %7s %8s %8s %6s" % ("wave", "event", "player", "level", "attacks", "hits", "blocked", "taken", "dodges", "stamina", "damage", "deaths"))
    for r in rows:
        if r["kind"] == "player":
            print("  %-6s %-13s %-8s %-6s %7s %5s %8s %7s %7s %8.0f %8.0f %6s"
                  % (r["wave"], r["event"], r["who"], r["level"], r["attacks"], r["hits"], r["hits_blocked"], r["hits_taken"], r["dodges"],
                     float(r["stamina_used"]), float(r["damage_dealt"]), r["deaths"]))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
