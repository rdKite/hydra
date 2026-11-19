# M5 preparation – what is ready for the human to decide and play

Prepared by the agent (roadmap M5, second item). **Nothing here is decided.** Each part says
what it is, how to look at it and which question it puts to the human.

## 1. Look

**Decided 2026-11-19: plain** – standard lighting, no toon bands, no ink outlines
(`docs/decisions.md`). The two other variants prepared for the comparison (toon, ink), their
post-process material and the comparison script were removed after the decision; they are
in the Git history up to commit `bd1da64`.

**Still open:** how much texture detail should the models get with the plain look –
procedural only, or hand-painted detail as well? The plain look relies on textures and
materials the most.

### The humanoid base model

One mesh and one skeleton for all humanoids (`docs/tech/blender-pipeline.md`), shown in the
game with placeholder animations made by script; the telegraph colours on the blade stay.
`Scripts/shots_humanoid.sh` shows the three body presets and the body with legs at 80 %
(idle, block, heavy attack charging) on one page: `Saved/Screenshots/Humanoid/index.html`.
It is a stand-in: no face, mitten hands, plain colour (tinted per side).

**Questions:** Are the proportions and the three presets a base you want to animate on?
Is the rig (bone names of the Unreal mannequin, A-pose, no fingers) right for your tools?

## 2. Which placeholder animations to replace first

Ranked by how much of the fight a player has to **read** from the animation. In this game
the wind-up is the telegraph – you react to it, not to the impact –, so the clips that
announce something come first, the ones that only decorate last. The game stretches every
action clip to the duration from `data/values.json`, so a clip's length in Blender does not
change the timing; its **readable silhouette** is what counts.

| Rank | Clip (name for the import) | Why | What the placeholder lacks |
|---|---|---|---|
| 1 | **Charge** (heavy attack, looped while held) and **HeavyStrike** | the longest and most dangerous telegraph (400–1200 ms, up to 600 damage of 1,000 HP): it must be unmistakable from the front and the side, and show that it is a thrust | a clear pulled-back pose that differs from the light wind-up at a glance; weight in the thrust |
| 2 | **Windup** and **Strike** (light attack) | the most frequent attack; 250 ms wind-up – short, so the first frames must already say "attack" and from which side the slash comes | anticipation in the first third; an arc that matches the hit area (70° to each side) |
| 3 | **Block** | shows the block arc: where the shield is, is where hits are stopped – flank and back are open | a stance that makes the covered side obvious; the shield clearly in front |
| 4 | **Recovery** | the opening after an attack: the opponent must see that the attacker is still bound | a visible "overextended" moment before returning to the stance |
| 5 | **Dodge** | pure movement without invulnerability: must read as a step, not a roll that suggests i-frames | direction of the step (the placeholder ducks on the spot) |
| 6 | **Idle** / stance and **Move** | carry the facing (weak point: the back) and are on screen most of the time, but announce nothing | a stance whose front is obvious; a run that does not slide |
| 7 | **Jump**, **Death** | feedback only | – |
| – | Slime: **Cast**, **CastCharge**, **CastRelease**, then **Windup** / **Charge** / **Strike** (slam) | same order of reasons for the slime: the bolt's charge and the slam's wind-up are its telegraphs | squash and stretch is there; the direction of the cast is not shown |

Until the human's animations exist, the telegraph colours on the blade stay (yellow in the
wind-up, towards red while a heavy attack charges).

**Path for the human's animations:** `docs/tech/blender-pipeline.md` ("Animations by the
human"). An action named like a clip above replaces the placeholder at the next
`Scripts/build_art.sh import`.

**Question:** Is this the order you want to animate in? Should the humanoid's light attack
get more than one clip (alternating sides in a chain)?

## 3. Playtest kit

| What | How |
|---|---|
| Start a session | `Scripts/playtest.sh humanoid` or `Scripts/playtest.sh slime` (one window against the waves); add `duo` for a second player window |
| Debug panel | **F1** in the game: form, level, every skill, fire imprint, open a milestone, reset, spawn any enemy type at a level (active or passive), remove all enemies, waves on / off. Also as console commands (`README.md`) |
| Numbers of a session | printed when the session ends, or `Scripts/summarize_metrics.sh` – hits, blocked hits, dodges, stamina use per player, enemy type and wave; the raw file is `Saved/Metrics/metrics_<time>.csv` |
| Change a number | edit `data/values.json`, press **F5** in the game (the host reloads and tells the clients) |
| Body presets and leg length of the humanoid base model | console `HydraBody neutral|male|female`, `HydraLegs 0.8` |

**Things worth trying in the playtest** (each is an open guess in the numbers):
- A light longsword strike costs half the stamina bar at level 0, a little under half at
  the start level 9. Does the rhythm "two strikes, then breathe" feel right?
- Holding the block costs 50 stamina per second and a blocked light hit 140 of about
  1,180. Is turtling punished enough, is blocking still worth it?
- A heavy hit interrupts (25 % of the target's HP), a light one never does between equals.
- The slime evolves at level 30 into a form that starts at level 0. Does that feel like a
  step forward within a 10–15 minute session?
- Three waves give the first level-up; hard enemy types give more skill points. Is the
  skill growth noticeable at all in one session?
- Enemy mix: is wave 3 (two ember slimes and a grunt) fair for a humanoid without ranged
  attacks?
