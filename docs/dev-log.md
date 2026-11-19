# Dev log

Newest entries at the top. Per session: date, start and end time, what was done per
roadmap item, **interventions** (manual editor steps by a human), assumptions, problems and
workarounds.

## Interventions

| Date | What did a human have to do? | Why couldn't it be done without? |
|---|---|---|

## Sessions

### 2026-11-19 – answers to the M5 questions, M4b, M5 preparation (agent session, start 19:10)

Pulled `main` (design sessions of 2026-11-16 to 2026-11-19) and followed the plan of the
entry below, in its order.

**Step 1 – the five answers to the M5 questions.**
- Skill XP is credited when an enemy is defeated, not when the wave is cleared; a defeated
  player loses only what is pending since then. (M4b turns this into hXP.)
- The bolt follows the camera from the press until it is out, at the turn rate while an
  attack binds – built like the raised block (new state tag `State.Casting`). An AI has no
  camera: it still turns to where it aimed the cast.
- Imprint thresholds 100 / 250 (values v6, set in the design session): nothing in the code
  or the live runs depended on the old numbers (the waves run sets its own in its data copy).
- Milestone keys: evolve **J**, imprint **K**; gamepad unchanged (D-pad up / right). HUD
  hint, README and `phase-1.md` §24/§25 updated.
- Verified: `Scripts/run_all.sh` green – 19 tests, combat run 153, slime run 80 (4 new: a
  cast charged for a second while the camera turns by 40 degrees leaves at 40 degrees),
  waves run 99.
- Problem: the waves run failed once in three places for reasons of its own timing (the
  first wave came 2 s after the host's start, which was after the client had joined when
  the host loaded slowly; the last grunt needs about 6.5 s to cross the arena and the run
  ended a moment too early). The run's data copy now uses a first delay of 0.1 s and the run
  is 3 s longer.

**Step 2 – M4b: align with the current design** (rules `docs/design/phase-1.md` §26, what was
built: §23–§25, rows marked *(M4b)*; numbers `data/values.json` v7, `data/CHANGELOG.md`).

- **Attributes.** Forms carry vitality, endurance, spirit, strength and dexterity; a level
  adds 2 % of the base to each; HP, stamina and mana are 10 per point. No other level bonus.
  `HydraCombatMath::BuildStats` works everything out from that, on server and client.
- **Costs** come from weight ÷ body weight (× demand) only; the slam uses its cost ratio.
  Skills and levels no longer lower a cost. **Regeneration** is a share of the bar per
  second and pauses while an action runs (attack to the end of its recovery, dodge with
  recovery, raised block; mana while a bolt is wound up or charged) – the regeneration delay
  is gone.
- **Damage:** base × (1 + skill bonus + strength / dexterity bonus by the weapon's fixed
  share [+ imprint bonus for fire]) × charge × chain × weak point, then the resistance curve
  (points). Weak point by dexterity. Base damage is stored per weapon and checked against
  the cost formula by an automation test.
- **Block:** a raised block costs stamina per second; a blocked hit costs the blocker a
  share of the blocked damage and the attacker a share as rebound. **Interrupt** at a share
  of the target's maximum HP.
- **Dodge and jump are capabilities:** the dodge skill is gone; distance and speed of the
  step grow a little with dexterity.
- **Skill growth:** hXP per defeated enemy (factor per enemy type), split by the stamina and
  mana each skill spent since the last defeated enemy; automatic skill points at each
  level-up by the hardship rating, split by the hXP of the level, with a progress bar per
  skill and the character level as the limit. The HUD shows the bars and the rating.
- Verified: `Scripts/run_all.sh` green – 19 automation tests (rewritten for the new rules),
  combat run 164 checks, slime run 83, waves run 125. The checkers work the expected numbers
  out with the new formulas; expectations changed on purpose wherever a rule changed.
  Examples from the runs: a light longsword strike costs 490 of 1,000 stamina and deals 200;
  a blocked light hit lets 60 through, costs the blocker 140 and the attacker 56 extra;
  holding the block costs 50 per second; a heavy hit (400) interrupts, a light one does not;
  the slime at level 29 has 948 HP and its slam deals 130.5; a grunt worth hXP = 2 × nXP
  gives three skill points, split 2.09 / 0.91 between slam and projectile by their use.

**Assumptions in M4b (to be confirmed by the human):**
- **A strike costs half the bar** (design target), so the scripted combat run refills the
  host's stamina between steps that test something else (new dev command `HydraRefill`).
- Dexterity base values: humanoid 100 (design), slime 80, fire elemental slime 90.
- Strength and dexterity grow alike in Phase 1 (no training), so a weapon's strength share
  changes nothing yet; it is in the data and the formula.
- The slam has no weight any more (cost from its cost ratio); its base damage uses the
  reference efficiency 0.41 (design: "about 95").
- The mana cost of the bolt is the same for everyone (skills give effect, not lower costs).
- A fire bolt's damage bonus adds projectile skill, fire magic skill and imprint; a bolt
  without an element only the projectile skill.
- Weak point and resistance are applied before the block, so "blocked damage" is what would
  have arrived. A hit that lands in a block can interrupt if what gets through is enough.
- Block at empty stamina: it stays raised (the holding cost just stops at 0) and the next
  hit breaks it – the design's "the block breaks: you take the hit".
- A blocked bolt costs the blocker by the same share rule as a strike; no rebound.
- No use, no points: if no skill was used in a level, its skill points are not handed out
  (the design says the hXP "go to no skill").
- Several level-ups from one enemy each give the points of that level's hardship rating.
- What a skill cannot take at its limit and nobody else can take waits on its bar (the bar
  shows full) and is used at the next level-up.
- Imprint thresholds 1,000 / 2,500: the v6 numbers × 10, because every damage number grew
  tenfold and the threshold was meant as "about one wave of fire contact".
- The live runs for combat and slime still run at level 0 without skill levels (their data
  copy), so they measure the base rules; the waves run starts at the real start levels.

**Problems in M4b.** The HUD bars ran off the screen with the tenfold numbers (seen on a
screenshot of the waves run, not by a check) – the bar scale is 0.3 pixels per point now.

**Step 3 – M5 preparation, humanoid base model: the Unreal side.** The human started a
second agent session for the Blender part (model, rig, shape keys, placeholder animations,
export) while this session had just begun it; this session handed it over and built the
side of the game:
- **`UHydraAnimInstance`** (C++, no Animation Blueprint): every form with a model is
  animated through it now – one clip per thing the game shows, time-scaled to the action,
  with a short cross-fade, plus bone lengths that can be scaled at runtime. The slime uses
  it too (the single-node playback of M3 is gone).
- **Clips** looked up in addition: `HeavyStrike` (else `Strike`), `Block`, `Jump`.
- **Sockets:** a model with the bones `weapon_r` and `shield_l` holds the placeholder sword
  and shield there (the blade still carries the telegraph colours); it takes the colour of
  its side through the material's Tint parameter.
- **Body presets and bone length** (pipeline test for the character creation): replicated
  per character, dev commands `HydraBody neutral|male|female` (morph targets `male`,
  `female`) and `HydraLegs <factor>` (thigh and calf shorter or longer, pelvis lowered by
  what the legs lost, girth unchanged) – both for every character at once.
- **Import:** morph targets are imported; the generated material is flagged for them.
- Verified with a local draft of the model (not committed): shown in the game with 11
  clips, block pose through the anim instance, male preset and legs at 80 % visible on
  host and AI (screenshots looked at). `Scripts/run_all.sh` green with the draft model
  (19 / 164 / 83 / 125) and the combat and slime runs green without it (the state of the
  repository until the Blender part lands).
- **Open until the Blender part is on `main`:** import it and commit
  `Content/Characters/humanoid`, set the orientation of sword and shield on the sockets by
  eye (they depend on the bone axes of the final rig), the screenshots of every variant in
  idle, attack and block, and a check in a live run.

**Step 3 – M5 preparation, playtest kit.**
- **Debug panel** (`docs/design/phase-1.md` §27, F1): form, level, every skill, fire imprint,
  open a milestone, reset, spawn an enemy type at a level (active or passive), remove all
  enemies, waves on / off. Drawn by the HUD class in C++, usable with keys, gamepad and
  mouse; the own character's input is paused while it is open. It only sends requests –
  the server applies them, also for the client window. Every action is a console command
  too (`HydraSetForm`, `HydraSetLevel`, `HydraSetSkill`, `HydraSetImprint`, `HydraMilestone`,
  `HydraReset`, `HydraSpawn`, `HydraClearEnemies`, `HydraWaves 0|1`). Compiled out of
  shipping builds with the dev RPC.
- **`Scripts/playtest.sh humanoid|slime [duo]`** starts a session against the waves and
  prints the metrics summary when the window is closed; **`Scripts/summarize_metrics.sh`**
  summarises a metrics CSV (per player, per enemy type, per wave).
- Verified: `Scripts/run_live_debug.sh` (40 checks) drives the commands on host and client
  and checks the result on both – set form, level (bars follow), skills (cut to the level),
  imprint, milestone and its offer, clear, spawn at a level in front of the caller (also
  from the client), reset, the panel driven like keys doing the same as the commands, an
  attack ignored while the panel is open, waves on and off. `Scripts/run_all.sh` green:
  19 tests, live runs 164 / 83 / 125 / 40. The summary was run on the metrics of the waves
  live run.
- Assumptions: enemies spawned by the panel do not count for a wave (they give nXP and hXP);
  "remove all enemies" in wave mode lets the next wave come after the usual break; the
  panel sets a skill's level, its progress bar only through the console command.
- Not verified by a human: mouse and gamepad in the panel (the live run drives it through
  the same functions as the keys, not through real input devices).
- Seen in the summary: a player who has already left when the session ends gets no last
  row (the client window of a live run closes at the same moment as the host).

### 2026-11-19 – skill growth comes to Phase 1; plan for an unattended session (design session)

**Next agent session – work through in this order without waiting for the human.** Commit
and push after each verified step (`Scripts/run_all.sh` green); assume and log where
something is open; after two serious failed attempts log it and go on.
1. The five items of "answers to the M5 questions" (2026-11-16) below. In item 1 "skill
   XP" becomes hXP in M4b; credit per enemy stays.
2. **M4b** (`docs/roadmap.md`), all items: the attribute model (entry 2026-11-17 below) and
   the new skill growth – rules in `docs/design/phase-1.md` §17 and §26. §26 includes the
   weapon rules of 2026-11-19 (longsword 1.5 kg, fixed strength / dexterity share, base
   damage from the cost, slam cost ratio about 0.01). Update §23–§25 to
   what was built.
3. **M5 preparation** (roadmap M5): first the **humanoid base model and rig** – gender-neutral,
   with the shape-key and bone-length pipeline test of the roadmap item (push the rig as
   soon as it imports and plays, so the human can start animating), then look
   variants, animation proposal,
   playtest kit including the **debug panel** (`docs/design/phase-1.md` §27). Decide nothing; put the questions for the human at the end of the log
   entry.

### 2026-11-17 – attribute model comes to Phase 1 (human, recorded in the design session)

To do at the start of the next agent session:
1. First the five items of the entry below (2026-11-16) – they still apply. Item 1 (skill
   XP per enemy) stays; the dodge skill it mentions goes away in M4b.
2. Then **M4b** in `docs/roadmap.md`: align Phase 1 with the attribute model of
   2026-11-17. Rules and starting formulas: `docs/design/phase-1.md` §26 (and §4–§17 as
   changed). Numbers in `data/values.json` (new version, guesses marked in
   `data/CHANGELOG.md`). Keep all live runs green; adjust their expectations where the
   rules changed on purpose and say so in the log. Update §23–§25 to what was built.
3. Where §26 leaves something open or a formula does not work in a live run: assume, log
   it, carry on.

### 2026-11-16 – answers to the M5 questions (human, recorded in the design session)

To do at the start of the next agent session, before M5 work:
1. **Skill XP per enemy:** credit pending skill XP when an enemy is defeated (not when the
   wave is cleared); on defeat only the XP pending since the last enemy defeat is lost.
   Rule: `docs/design/phase-1.md` (skill XP).
2. **Fire bolt charge follows the camera** at the attacking turn rate, like a raised block.
3. **Imprint thresholds** changed in `data/values.json` v6 (100 / 250) – check the waves
   live run still passes and adjust its expectations if it hard-codes the old numbers.
4. No change for: evolution in the arena (no workaround), 1 % per level/skill (stays), full
   nXP for everyone (already so).
5. **Move the milestone keys away from WASD:** evolve **E → J**, imprint **R → K**
   (keyboard). E and R stay free for later actions. Keep the gamepad mapping unless it
   clashes; update the HUD hint and any docs that name E/R.


### 2026-11-16 – precise movement, then M4 (agent session, start 18:55)

**Before M4 – feedback from the human's first hands-on test.** The character always turned
into its movement, so stepping backwards was impossible without lock-on. Built:
- **Precise movement**, a toggle on **X** (gamepad: left stick click), shown on the HUD. It
  sets the same strafe flag as lock-on, without a target: the character faces the camera
  direction at its turn rate and moves at `movement.strafe_speed_m_s`. Dev command
  `HydraPrecise 0|1`.
- **A raised block faces the camera direction by itself** (asked for by the human while this
  was being built). Movement speed while blocking is unchanged (run speed ×
  `block.move_speed_ratio`).
- Assumptions: X as a toggle rather than holding Q (Q is pressed by a finger that also
  moves the character; easy to change in `AHydraPlayerController::BuildInput`); precise
  movement uses the strafe speed, like lock-on; no aim assist in precise movement (as with
  lock-on); the toggle survives death and respawn. No new values.
- Not built: the same for the bolt's charge (the cast still turns to the direction of the
  press) – the human named "drawing a bow" as a later case; the bolt is the nearest thing
  today. To decide.
- Verified: `Scripts/run_all.sh` all green (17 tests, combat run 153 checks – 7 new: one
  second backwards in precise movement keeps the facing within 3° on server and client and
  covers about the strafe speed, without it the character turns around; a blocker without
  lock-on turns to the camera direction at the blocking turn rate; slime run 76).

**M4 – Progression and waves.** Rules as built: `docs/design/phase-1.md` §25; numbers:
`data/values.json` v5 (all guesses, `data/CHANGELOG.md`).

**Item 1 – character level, skill XP by use, one imprint, fixed start levels.**
`UHydraProgressionComponent` on every character (server decides, replicated): level and
nXP, skills with credited and pending XP, fire imprint level and progress, open milestones.
`HydraCombatMath::BuildStats` takes the progress and works out the numbers (HP and damage by
level; damage, cost, block arc, dodge step, field size by skill; fire resistance and fire
damage by imprint), so server and client use the same. The HUD shows form, level, XP bar,
skills with their XP, imprint progress and what a milestone offers; keys E (evolve) and R
(imprint).
Verified live (`Scripts/run_live_waves.sh`): slime starts at level 29 with skills at 20,
humanoid at 9 / 6; a dodge that takes the slime out of a grunt's attack counts for the dodge
skill, one into the air would not; two slams deal weapon × level × skill damage and cost
less by the skill; skill XP is pending during the wave, credited when it is cleared; the
grunt's nXP takes the slime to level 30 and the humanoid to 11 as the XP curve says; the slam
skill rises by one, the dodge skill keeps its XP for later; contact with fire (the ember
slime's element body) fills the imprint progress; the client starts its imprint at its open
milestone through the RPC and gets fire magic at level 1.

**Item 2 – evolution to the fire elemental slime with an instinct.** New form
`fire_elemental_slime` (own numbers, the slime's model tinted and glowing through two
material parameters the import script now adds; element body: fire damage back on melee
hits). Evolution is offered at an open milestone or at the level cap once the fire imprint is
there or earned.
Verified live: at level 30 with the imprint earned the HUD offers both; after E: fire
elemental slime, level 0, skills 0, fire imprint 1, fire magic 1, full bars of the new form
(80 HP, 130 mana), instinct 0.70 for slam (21 of 30) – the slam costs 14.69 instead of 15
stamina at skill level 0; the next slam deals level-0 damage with that bonus; the new form
levels again from 0; server and client agree; the model shows orange (screenshot
`Saved/Screenshots/LiveWaves/host_16.8.png`).

**Item 3 – AI enemies in waves.** `enemies.json` (grunt, veteran, ember slime) with level,
skill level, nXP and AI numbers per type in `values.json`; five waves in `waves.list`,
looping with stronger enemies. Wave mode is the default of `Scripts/play.sh` (`WAVES=0` for
the practice opponent; the two older live runs still use that). Enemies are ordinary
characters with the AI controller of M2, which now reads its numbers per enemy type, casts
(ember slime: fire bolts from a distance, slam up close), steps around obstacles and casts
only with a clear line. AI characters do not hurt each other.
Verified live: waves come as the data says, the second after the break; enemy level, HP and
form are right and replicated; the element body deals 4.2 fire damage back per hit; a
defeated enemy is removed; switched to active the remaining grunt comes for the players.
Separately, by hand-started probe runs with active enemies: ember slimes hit a standing
player with fire bolts from about 7 m, leave fire fields and cause burning; one that spawned
behind the low cover walked around it (before the fix it stood there and shot the cover).

**Item 4 – metrics as CSV.** Counters per character on the server; the game mode appends one
row per player and per enemy type at every cleared wave and at the end of the session
(`Saved/Metrics/metrics_<time>.csv`, `-HydraMetricsDir=` overrides).
Verified live: the file of the run has the expected rows; wave 1: host 2 attacks, 2 hits, 1
dodge that avoided a hit, 1 kill, stamina use = one dodge plus two slams to 0.2, damage
dealt as logged; grunt 1 attack, 1 miss, 2 hits taken, 1 death; the rest at session end.

**Verification.** `Scripts/run_all.sh`: 19 automation tests, combat run 153 checks, slime run 76, waves run 99 - all green.
The same from a **fresh clone** of `main` after the push (build from nothing, LFS assets
from the remote): all green.

**Assumptions (to be confirmed by the human):**
- Humanoid level cap 50 (the design names none). Start levels 9 (humanoid) and 29 (slime).
- Every player in the arena gets the full nXP of a defeated enemy.
- "Only on victory": skill XP of a wave is lost when the player is defeated in it; the wave
  goes on, the player respawns as before.
- The last enemy of a wave credits skill XP first and gives nXP second, so that level-up can
  raise skills at once.
- A milestone stays open until it is used; evolution is also possible at the level cap
  without an open milestone (otherwise a slime that spent its last milestone on the imprint
  could never evolve).
- Imprint progress is kept only up to what the next level needs.
- Instinct = (old skill level ÷ old level cap) × 3 skill levels for good and up to 50 % more
  skill XP; the highest ratio ever reached is kept.
- After the evolution fire magic is 1 although the level is 0 (awakening beats "skills
  never exceed the level").
- Element body: fire damage back on every melee hit that lands, blocked or not, scaled by
  the level of the body.
- AI characters do not hurt each other with attacks and bolts; their fire fields burn
  everyone.
- Dodges count only against melee attacks, not against bolts.
- The practice mode gives no nXP and credits no skill XP (progression runs in waves).
- The two older live runs read a data copy with start level 0 and without imprint bonuses,
  so they keep measuring the plain rules; the waves run reads a copy with low thresholds and
  two small waves. The real `data/values.json` is checked by the automation tests (three
  waves give enough nXP for a level-up of either form).

**Problems and workarounds.**
- The first waves run failed in one place by its own geometry: the scripted dodge ended in
  the low cover next to the spawn spot. The script now approaches from the side.
- An enemy that spawned behind the cover stood still (the AI walked straight at its target)
  – found in a probe run, not by a check. Fixed with a short probe ahead and a step to the
  free side; no navigation mesh.
- `Scripts/run_all.sh` waited five minutes after every live run: the watchdog's `sleep`
  kept the output pipe open after the run had ended. Its output is detached now.
- Compiled Python files (`__pycache__`) had been committed with M3; removed from git.

**Interventions:** none.

**Ideas noted, not built:** a choice menu for milestones; bolts counting for the dodge
skill; an end-of-wave summary on the HUD from the same counters as the CSV; enemies that
lead their target with the bolt; per-enemy looks (they are told apart by the name above
their bars); a wave enemy of the plain slime form.

**To decide before M5 (human):**

1. **Is the evolution worth it in the arena?** A slime at level 30 with skills around 20
   becomes a fire elemental slime at level 0 with skills at 0 – as the design says, but in a
   10–15 minute session it is weaker for most of what is left (base values are higher, level
   and skill bonuses are gone, the instinct gives back about 2 skill levels). Options: leave
   it (the long game pays it back), a stronger instinct, or higher base values.
2. **Strength of levels and skills:** 1 % HP and damage per level, 1 % damage and cost per
   skill level. A level-29 slime with skills at 20 hits about 55 % harder than a new one. Too
   much, too little?
3. **nXP for everyone in full** – or split between the players?
4. **Losing a wave's skill XP on defeat** – right reading of "only on victory"?
5. **Imprint speed:** two fire bolt hits (about 36 fire damage) are enough for the first
   level; the milestone is the real gate. Fine, or slower?
6. **Enemy mix and difficulty** are untested by a human. Standing still, a level-9 humanoid
   lasts about 6 seconds against wave 3 (two ember slimes and a grunt).
7. **Bolt charge and precise movement** (from the feedback before M4): should the caster
   follow the camera while it charges, like a raised block?
8. **Keys:** X for precise movement, E and R at a milestone – fine?

Session end: 19:46.

### 2026-11-16 – M3 Slime and magic (agent session, start 16:40)

Before M3, answers of the human to the M2 questions: **jump on Space, dodge on Left Alt**
(done; walk moved to Left Ctrl – assumption; `movement.jump_height_m`, values v3), **feints
out of the plan**, **no Mixamo interim look**, **block state stays lag-compensated**. Pulled
`main`: animations are designed by the human, textures are procedural from Blender scripts
– both fit what M3 builds. `UE_ROOT` is still not set on this machine.

**Item 1 – Blender pipeline by script.** `Art/scripts/` (shared helpers, one build script
per model, `export_animations.py`), `Scripts/build_art.sh` (Blender headless, then the
Unreal import commandlet `Scripts/import_art.py`). Scene unit centimetre, fixed FBX
settings, one FBX per action, material built on import. Settings, commands and the path for
the human's animations: `docs/tech/blender-pipeline.md`. Blender 5.2.3 was already
installed; nothing was installed by the agent.
Verified: both scripts run headless; import log reports slime 83 × 83 × 46 cm with its base
on the floor, 3 bones, 11 animations; wolf pup 18 × 67 × 45 cm, 15 bones;
`export_animations.py` exports all 11 actions from a copy of `slime.blend`.

**Item 2 – "models by script" test.** Slime: built, rigged, textured, animated with
placeholders, imported and **playable** (`HOST_FORM=slime Scripts/play.sh`). Wolf pup:
model, rig, texture and a test pose, imported, not playable. The answers to the three
questions are in `docs/tech/blender-pipeline.md`; in short: simple shapes work well by
script, a four-legged body only as a rough stand-in (clean quads, bends without tearing,
but no character), no human step needed for the pipeline.
- The game shows a form with its model if `/Game/Characters/<form>/SK_<form>` exists and
  plays one clip per action in single-node mode, time-scaled to the action's duration from
  `values.json`. **Assumption / deviation:** this is not the blending C++ anim instance of
  the tech test – a slime with eleven short placeholder clips does not need blending. The
  blending instance comes when the humanoid gets animations.
- Preview renders of every build: `Saved/ArtPreview/` (not in git).

**Item 3 – slime: slam, projectile, mana.** Form numbers (size, mana, resistances) and the
slam as a body attack in `values.json` (v4). The slam reuses the attack ability; the
projectile is `UHydraGA_Cast` (mana, wind-up, charge by holding) and spawns
`AHydraProjectile`, which the server alone simulates (clients fly a copy along the same
line). A form is chosen per player by a start option (`?Form=slime`) or the dev command
`HydraForm`. Mana is a new attribute with its own regeneration.
Verified live (`Scripts/run_live_slime.sh`, 76 checks): slam 14 damage after 300 ms for 20
stamina, charged slam 39; the AI's sword does 10 instead of 20 to the slime (slash
resistance); bolt 15 damage for 20 mana, fully charged 24 for 36, flight time for 6 m as
`bolt.speed_m_s` says; client sees form, bars and model.

**Item 4 – fire bolt, fire field, burning.** With fire awakened the bolt deals fire damage,
adds a burning stack and leaves `AHydraElementField`. Burning follows the design (stack up
in the field per interval, down outside, damage per stack) and is kept on the character.
Verified live: fire field of 1 m for 3 s under the target; the target burns (server and
client see the stack), a second character stepping into the field burns at once; both
stacks drop 10.0 s later; 10 damage per stack over its interval; the caster does not burn;
against a block the bolt bursts on the shield (4.5 of 15 damage, blocker pays 24 stamina,
no stack, field 135 cm in front of the blocker); a cast aimed 90° to the side leaves at
about 65° because the slime turns at its turn rate.

**Verification.** `Scripts/run_all.sh`: 17 automation tests, combat live run 146 checks,
slime live run 76 checks. A fresh clone of `main` (with the generated assets from LFS, no
Blender run needed) gives the same: 17 / 146 / 76, all green. Session end: 17:35.

**Assumptions (to confirm).**
- All new numbers are guesses (`data/CHANGELOG.md` v3, v4). Formulas: `phase-1.md` §24.
- Slime buttons: weapon-hand button = slam, off-hand button = bolt.
- The projectile (body skill) *is* the bolt (spell form): without an element blunt damage,
  with fire the fire bolt. Fire is switched by a start option / dev command until M4 earns
  it by an imprint. Default: off.
- The bolt flies level and straight (no gravity), is aimed by camera yaw only, and the
  caster turns towards the aim at its turn rate. No aim assist for it yet.
- Bolt against a block costs the blocker `block.stamina_cost_per_hit` (a bolt has no
  weight); a blocked fire bolt adds no burning stack.
- Burning: entering a field gives the first stack at once; the stack timer is one timer for
  up and down. Burning damage is fire damage and goes through fire resistance.
- Jump: no stamina cost; not while an attack binds, while blocking or dodging.
- Movement speeds are still the same for all forms.
- Burning is not a GAS effect but state on the character (stack count replicated, damage
  through the damage effect): the design's up/down stacking does not map well onto effect
  stacking, and the tech test's version needed class-default mutation.

**Not built (noted):** gravity for the bolt; lag compensation for the bolt (server-side
flight, 20 m/s); effects for burning beyond HUD text; slime AI (the AI opponent stays a
humanoid and does not react to bolts in particular); form selection menu.

**Problems and workarounds.**
- Blender: objects removed a moment ago are still in the view layer list as `None`
  (update the view layer before selecting); `use_nodes` is deprecated in 5.2 (guarded).
- The imported material was grey in the game: skinned meshes need the usage flag set at
  import, a game build cannot add it.
- Other players' slime floated 40 cm: character network smoothing resets the mesh to a
  cached offset → `CacheInitialMeshOffset` after placing the model.
- A tapped cast got a sliver of charge from frame timing (15.1 instead of 15 damage); the
  charge is now decided together with the release.
- Two checks sat exactly on a 2 s chain boundary and flickered; moved.

**Interventions:** none.

**To decide before M4 (human):**
1. **Slime vs. the AI opponent:** the active AI kills a standing slime in about five
   seconds (60 HP, chained sword hits). That is "very weak" as designed – is it the wanted
   starting point, or should the practice AI start passive / gentler?
2. **Bolt flight:** level and straight, 20 m/s, no gravity. Keep for Phase 1 or add an arc
   ("like a bow shot")?
3. **Burning numbers:** one stack = 10 damage over 10 s; a field only ever adds one stack
   by itself because it lasts 3 s and the interval is 10 s. More stacks come only from more
   bolts. Intended, or should fields last longer / stack faster?
4. **Per-form movement speed:** the slime moves exactly like the humanoid. Add speeds per
   form to `values.json`?
5. **Wolf pup quality:** the scripted quadruped is a stand-in. Fine as the pipeline test it
   was meant to be – who makes the real wolf in Phase 1b (script, human, or a mix)?

### 2026-11-16 – M2 Combat core, humanoid (agent session, start 15:50)

Answers of the human to the M1 questions: per-form and per-weapon numbers first "into
`forms.json` / `weapons.json`", then during the session **"all values into `values.json`
after all"** (done that way: sections `forms`, `weapons`, `shields` keyed by ID); aim assist
must respect the turn rate; 120 fps cap is fine; the dummy stays until the AI opponent
replaces it. `UE_ROOT` is still not set on this machine (passed per command again).

**Data (values v2).** `FHydraValues` got maps keyed by ID; the key checks now also walk into
those sections; new check `Hydra.Data.NumbersForEveryId` (every form, weapon and shield has
numbers, no numbers for unknown IDs). `HydraCombatMath::BuildStats` turns a form into
`FHydraCombatStats` (weight = body plus load, weight ratio, costs, turn rate). Details of
every number: `data/CHANGELOG.md` v2. Formulas: `docs/design/phase-1.md` §23.

**Item 1 – longsword and shield, one button per hand, light and heavy, chains, interrupt,
weight costs stamina.** `UHydraGameplayAbility` (base: locally predicted, one instance per
character, timers) and `UHydraGA_Attack`: every attack starts with the light wind-up; still
held when it is over → heavy attack (charge, strikes on release, not before the minimum
wind-up, or at full charge). Hits are resolved in `HydraCharacterCombat.cpp`. Health is a
new attribute; death and respawn after `respawn_delay_ms` at the spawn point.
Verified live: three light hits 20 / 22 / 24 with chain 1-2-3, chain gone after 2 s, heavy
released early = 40 at 400 ms, full charge = 60 at 1200 ms, attack costs 25 / 40, a light
hit (20) does not interrupt a charging humanoid (threshold 24.45) and its attack lands, a
heavy hit (40) does and the attack never lands; AI defeated and back after 3.0 s.

**Item 2 – directional block, dodge, rebound, telegraphs.** `UHydraGA_Block` (hold; no
regeneration, slower movement), `UHydraGA_Dodge` (predicted root-motion step, then
recovery; nothing else – no invulnerability). Verified live: hit from the front into the
block 6 damage instead of 20, blocker pays 15 (light) and 24 (heavy), attacker rebound 6,
no regeneration while blocking, hit from behind ignores the block, block break at 5
stamina (hit lands in full, stamina 0); the AI attacks, the host dodges 3 m back and the
attack is a miss while an undodged one hits.

- Telegraphs with the placeholder body: sword and shield from primitives, posed by the
  action (wind-up cocked back, heavy charge pulled back, strike, block with the shield in
  front); the blade is yellow in the wind-up and turns red while a heavy attack charges;
  short white flash on a hit, blue on a block. The action is replicated
  (`FHydraActionState`); the owning client shows its own predicted actions at once.
- The camera now sits higher over the right shoulder: with the M1 camera the own body hid
  an opponent at sword distance.

**Item 3 – turn rate.** `UHydraMovementComponent` computes the rate every tick from weight
ratio, block and attack state (tags) inside floor and cap, and every turn goes through it:
movement direction, lock-on, and aim assist (a facing goal during the attack). Verified
live with the turn rate lowered by a reload: lock-on turn 63°/s free, 31°/s while blocking;
aim assist reaches only what the attack turn rate allows (11° of the 14° it wanted).
Unit test: slime vs. humanoid with shield differ clearly (720 vs. 565°/s).

**Item 4 – weak points.** Back arc, fixed multiplier, scaled by weapon weight ÷ strength.
Verified live: light hit on the back 30 instead of 20.

**Item 5 – hit detection with lag compensation.** Geometric test on the server (reach to
the body, arc), every tick during the strike. `FHydraPoseHistory` per character (position,
facing, block state; 0.5 s); the targets are rewound by the attacker's ping (capped by
`lag_compensation.max_rewind_ms`). Verified live at latency step 150 ms (measured ping
170 ms), both from one run:
- position: the host runs away from the client; the client's attack is timed by what the
  client sees. Server: rewound by 169 ms the target is 238 cm away → hit; at the server's
  present it was at 322 cm, outside the reach of 255 cm.
- block arc: the host raises its block 170 ms after it sees the client's attack start. On
  the server the block is up when the hit lands, but not at the moment the attacker saw →
  the hit is not blocked.
- `Hydra.LagCompensation 0` switches it off for comparison.

**Item 6 – AI opponent.** `AHydraAIController` plays an ordinary character through the same
inputs as a player (attack button, block button, dodge): walks up, keeps facing the player,
attacks light or heavy, reacts to attacks it sees after its reaction time with block or
dodge or not at all (error rate). Passive mode (`Hydra.AI 0`, `-HydraAIPassive`) for
scripted runs and quiet testing. Verified live: switched to active it walks 6 m to the
player, faces it, attacks and pays stamina; in one run it defeated the standing host and
went for the other player.

**Verification.** `Scripts/run_tests.sh` 16/16; `Scripts/run_live.sh` 144/144 (about 70 s;
the checker works the expected numbers out from `values.json` by itself). A fresh clone of
`main` builds and gives the same results (16/16, 144/144). The final version of the
live run passed both times it ran (working copy and fresh clone); earlier versions had
failures that were fixed (see problems below). Its AI-active part is not deterministic and
is checked loosely.
M2 came as two commits (code, then scripts and docs) instead of one per item: the combat
pieces depend on each other and would not have built separately. Session end: 16:45.

**Assumptions (to confirm in playtests or by decision).**
- Units: weights and strength share a unit; ratio 1 = rules unchanged. All new numbers are
  guesses (`data/CHANGELOG.md` v2).
- Heavy attack: starts as a light one; the light cost is paid at the press, the difference
  when it turns heavy. Light or heavy is decided on the server by when the release arrives
  there; right at the 250 ms boundary client and server can disagree for one attack (the
  server wins).
- Charge: damage grows linearly to ×1.5 at full charge; the cost does not grow.
- Chain: blocked hits get the chain multiplier but do not advance the chain; being
  interrupted ends your chain; being hit otherwise does not.
- Interrupt works while the target's attack binds (wind-up, charge, strike), not in its
  recovery.
- Block cost and rebound scale with attacking weapon weight ÷ shield weight. Strength of
  the blocker does not lower the block cost yet (the design mentions it).
- Lag compensation rewinds by the measured round trip (no extra term for interpolation);
  the block state counts as the attacker saw it, which favours the attacker by design.
- Aim assist: the facing goal is fixed at the start of the attack and replaces steering
  by movement input for that attack; nothing while locked on.
- Movement while an attack binds: 30 % speed (`attack.move_speed_ratio`).
- AI: no navigation mesh, walks straight at the target; targets players only.
- Hit stop is cosmetic (pose freeze), the timeline of the attack is not stretched.

**Not built (not on the M2 list, noted):** feints (cancel an attack with the other hand's
button), block with the weapon, stamina cost for running or sprinting, real sound, hit
react animation. `block.weapon_half_arc_deg` is in `values.json` but unused.

**Problems and workarounds.**
- The first screenshot of a process stalls it for about 350 ms and delayed a running
  attack. The live run now takes a warm-up shot at 0.3 s and only shoots in quiet moments.
- Lock-on right after a scripted camera turn in the same frame found no target (the camera
  view is one frame old). The live run leaves 0.3 s between the two.
- The auto-attack used for the lag-compensation check under-estimated the distance while
  the target was still accelerating; start distance and trigger were chosen with margin.
- Shell: here-documents with apostrophes fail in this environment; edit scripts are written
  as files first.

**Interventions:** none.

**Ideas noted, not built:** Mixamo assets from `C:\code\tmp\engine-test` (X Bot, Y Bot,
dodge, great sword pack) as an interim look – offered by the human; not taken over in M2
because it needs its own animation code that M3 (Blender pipeline) replaces. Stats overlay
for playtests. A second weapon to see the weight rules differ.

**To decide before M3 (human):**
1. **Feints** are in the design (§7, §8) but on no roadmap milestone. Add them to M2 as a
   follow-up, or to M4 with the smarter AI?
2. **Mixamo interim look:** take the Mixamo character and animations for M2–M4 playtests,
   or stay with the placeholder until our own models exist (M3)?
3. **Block and the attacker's view:** a block counts only if the attacker could see it
   (lag compensation). At 150 ms that makes a late block fail. Keep, or give the defender
   the benefit?
4. Starting numbers that shape the feel most and are pure guesses: interrupt threshold
   (a single light hit never interrupts a humanoid), block cost 15 / rebound 6, AI reaction
   300 ms and error rate 25 %.

### 2026-11-16 – M1 Foundation (agent session, start 14:40)

**Environment check.** Engine 5.8.4 (launcher build) at `D:\dev\Epic Games\UE_5.8`,
`Build.bat` and `UnrealEditor-Cmd.exe` present, Visual Studio 2026 (MSVC 14.52, Windows SDK
10.0.26100) builds a throwaway C++ project, Git LFS 3.8.1 installed.
**`UE_ROOT` was not set** (not in the process, user or machine environment). The engine sits
at the path `CLAUDE.md` gives as the example, so the session passed `UE_ROOT` per command and
went on; the system environment was not changed. The scripts stop with a clear message if
`UE_ROOT` is missing (`Scripts/env.sh`).

**Item 1 – project from text files.** `Hydra.uproject`, `Config/Default*.ini`, two targets,
module `Source/Hydra/` written by hand, no editor wizard. Plugins: GameplayAbilities,
EnhancedInput, PythonScriptPlugin (editor only). Build: `Scripts/build.sh`.

- Problem: the editor adds an `AndroidFileServer` block with a random token to
  `DefaultEngine.ini` on every first start. Workaround: plugin disabled in the `.uproject`.

**Item 2 – values subsystem.** `UHydraValuesSubsystem` (game instance subsystem) loads
`data/values.json` into `FHydraValues` (snake_case members, mapped 1:1 by
`FJsonObjectConverter`); console command `Hydra.ReloadValues`. Lookup: `-HydraDataDir=`,
then `data/`, then `Config/Hydra/` (build copy for packaged builds). Tests
`Hydra.Values.*`: load, every key mapped, every struct member present in the file, reload
through the subsystem (edited value is live, broken file keeps the old values).
Verified: `Scripts/run_tests.sh` passes; with a stray key added to `values.json` the run
fails with `key not read by the game: stamina.typo_key` (exit code 1).

- Beyond the roadmap wording: the reverse check (struct member without a key) – without it a
  removed key would silently run as 0. Stray and missing keys are also logged as warnings
  at runtime on every (re)load.

**Item 3 – data files with stable IDs.** `data/elements.json` (fire), `data/skills.json`
(sword_fighting, shield_fighting, dodge, fire_magic), `data/weapons.json` (longsword,
round_shield), `data/forms.json` (humanoid, slime); structs in `HydraDefinitions.h`, loaded
and reloaded together with the values. Validation at load and in tests: IDs are lower
snake_case and unique per file, references between files resolve.

- **Assumption:** the definition files hold identity and relations only (names, line, family,
  damage types, loadout, skills). **All numbers stay in `values.json`** for now, as
  `phase-1.md` says. Where per-form and per-weapon numbers go (weight, strength, level cap)
  is open – see "To decide before M2" at the end of this entry.
- **Assumption:** skills got first entries too (the four skills Phase 1 names), because
  weapons and elements refer to them.
- **Assumption:** ID format is plain lower snake_case without a namespace prefix. A prefix
  for mods (`hydra:slime`) can be added when mods come (Phase 2).

**Item 4 – GAS helper layer.** `Source/Hydra/Abilities/`: `UHydraGameplayEffect` (base class;
an effect is a subclass whose constructor calls `MakeInstant/MakeTimed/MakeInfinite`,
`AddAmountTo`, `GrantTag`, `PauseWhileTargetHas`, `StackPerTarget`), native tags in
`HydraTags`, `HydraGas::Apply` (builds context and spec, sets the SetByCaller magnitudes
`Data.Amount` and `Data.Duration`), `HYDRA_ATTRIBUTE_ACCESSORS`. First users: the three
stamina effects (change, regeneration, regeneration delay) and `UHydraAttributeSet`.
Verified: test `Hydra.Stamina.CostRegenDelay` runs the real effects on a real ability system
component in a throwaway world with the numbers from `values.json` (cost, no regeneration
during the delay, a second action restarts the delay, rate, clamp at 0 and max, new numbers
after a reload).

- Taken over from the tech test: effects as native classes, GE components as default
  subobjects, the deprecated `StackingType` member (the setter is not exported in 5.8).
- Regeneration runs in fixed 0.1 s ticks (`UHydraGE_StaminaRegen::TickS`). Treated as a
  technical constant, not a gameplay value: the rate per second comes from `values.json`.
- No ability base class yet – M1 has no abilities; it comes with the first attack in M2.

**Item 5 – host mode and network emulation.** `AHydraGameMode` spawns players at the arena's
spawn points; a second window joins with `127.0.0.1` (`Scripts/play.sh`).
`AHydraGameState` replicates the latency step and the packet-loss switch; every process
applies them to its own net driver (engine packet simulation): outgoing lag = step ÷ 2
(steps are round trip), loss = `packet_loss_ratio` per direction. Console
`Hydra.LatencyStep [ms]`, `Hydra.PacketLoss [0|1]`, keys F6 / F8, from host or client.
Verified in the live run: both sides log the applied setting; measured ping 12 ms without
emulation, about 120 ms at the 100 ms step (frame time on both ends is included); the client
switched packet loss off through the server.

- **Problem:** with two game windows on one machine the window in the background dropped to
  8 fps (the host, as soon as the client window had focus). Not a network effect: both
  windows rendered uncapped (~250 fps) and Windows gives the foreground window the GPU. The
  low host frame rate also inflated the ping (83 ms without emulation). **Workaround:** frame
  cap of 120 fps, `[Hydra] MaxFPS` in `Config/DefaultGame.ini`, applied in
  `AHydraPlayerController::BeginPlay`. A plain `t.MaxFPS` in an ini does not hold: the
  engine's game user settings overwrite it at start with their own `FrameRateLimit`.
- Packet loss switches on with the first latency step above 0 and off at step 0 (as in the
  tech test); `packet_loss_both_directions: false` makes only the server drop packets.
- Network emulation needs `DO_ENABLE_NET_TEST`, so it is not in shipping builds.

**Item 6 – arena.** `AHydraArena`: 40 m square, border wall, three pillars, two low covers, a
block and a wall segment, own lighting, all from engine basic shapes in C++. The map
`Content/Maps/Arena.umap` holds just this actor and is written headlessly by
`Scripts/create_arena_map.sh` (Python commandlet). Verified: screenshots of the live run.

**Item 7 – camera, soft aim assist, lock-on.** Third-person camera over the right shoulder
(spring arm on `AHydraCharacter`). Enhanced Input actions and the mapping context are built
in C++ (`AHydraPlayerController`, binding table in the header), mouse/keyboard and gamepad.
Lock-on (Q, middle mouse, right stick click) picks the best character in the cone around the
camera direction (`lock_on.*`), then the controller turns **only the camera** to the target;
the character follows the camera direction through the movement component at its turn rate.
The lock state travels in CharacterMovement's saved move (`FLAG_Custom_1`, as in the tech
test), so the server uses the same facing. Verified in the live run: both players lock on,
camera on target, server has the client's facing; then, with the turn rate lowered to 60°/s
by a reload, the camera is on the target after 0.5 s while the character has turned only
32° and catches up later. Pure rules (cone, score, aim-assist yaw, turn-rate floor and cap)
are unit-tested (`Hydra.Camera.*`).

- **Assumption (aim assist):** as in the tech test, aim assist is a one-off partial turn
  towards the best target in the assist cone (`aim_assist.angle_deg`, `range_m`) by
  `strength`, meant for the start of an attack. M1 has no attack, so it is implemented and
  tested (`AHydraCharacter::ApplyAimAssist`) and can be triggered with the console command
  `HydraAimAssist`; M2 calls it from the attack. This turn is instant, i.e. **not limited by
  the turn rate** – to decide, see below.
- **Assumption (turn rate):** M1 uses `turn_rate.base_deg_s` inside floor and cap for every
  character. Weight ÷ strength and the slowdown while blocking/attacking are M2.
- **Beyond the roadmap wording:** one passive **training dummy** (a character without a
  controller) stands in the arena, so lock-on and aim assist can be tried in a single window.
  It is not the AI opponent of M2.
- The character is a placeholder from engine primitives (cylinder, sphere, a dark visor that
  shows the facing; own character blue, other players red, dummy sand). No Mixamo assets were
  taken over; models come with the Blender pipeline in M3.
- Camera numbers (arm length, shoulder offset, lag, lock-on follow speed, gamepad look
  speed) are in code as camera feel, not in `values.json`.

**Item 8 – stamina.** Maximum, regeneration and regeneration delay from `values.json`
through the effects of item 4, on every character; `HydraGas::CanAfford` /
`HydraGas::SpendStamina` are what actions will call from M2 on. HUD: own bar (its length
grows with the maximum) and bars over other characters. Verified: automation test (item 4)
and live run – cost on the server, replicated to the owner and to the other player, no
regeneration during the delay, regeneration, new maximum after a reload on both sides.

- M1 has no action that costs stamina. For testing there is the console command
  `HydraSpendStamina <amount>` (server RPC, compiled out of shipping builds).
- "Blocking stops regeneration" is prepared (`State.StaminaRegenBlocked`) and used in M2.

**Item 9 – scripts.** `Scripts/run_tests.sh` (build + 11 automation tests, headless),
`Scripts/run_live.sh` (host + client window, automated input through
`UHydraDevSubsystem`: `-HydraExec`, `-HydraShot`, `-HydraQuit`, `-HydraWaitPlayers`; 68
checks from the logs in `Scripts/check_live.py`, four screenshots),
`Scripts/play.sh` (play by hand). Result of the last run: tests 11/11, live run 68/68.

- The dev clocks of host and client start when both players are in the game, so the script
  times of the two windows line up to about 0.1–0.2 s.
- The live run reads a copy of `data/` (`-HydraDataDir=Saved/Live/data`) and edits that copy
  for the reload check, so the real data files are never touched.
- `check_live.py` runs with the Python that ships with the engine (no extra install).
- Problem: Git Bash rewrites arguments such as `/Game/Maps/Arena` into Windows paths.
  Workaround in the scripts: `MSYS_NO_PATHCONV=1`.
- Problem: a first attempt to write several files with one long shell command failed
  (command line too long on Windows); files are written one by one.
- Final check on a **fresh clone** of `main` (includes the LFS map): build, tests 11/11 and
  live run 68/68. It found one bug – `Scripts/build.sh` failed without an existing `Saved/`
  folder – fixed.

**Not done / left open in M1:** nothing from the roadmap list. No packaged build was made
(not asked for in M1; `Hydra.Build.cs` already stages the data copy for it).

**Interventions:** none. No manual editor step was needed; the editor UI was never opened.

**Ideas noted, not built:** per-form data (speeds, stamina) instead of one global set – M2
needs it for weight and strength; stats overlay key for playtests; a `Scripts/setup.sh` that
checks the environment for a new machine.

**To decide before M2 (human):**

1. **Where do per-form and per-weapon numbers live?** (weight, strength, level cap, stamina
   and mana per form, reach and damage per weapon). Options: sections in `values.json` keyed
   by ID (`"forms": {"slime": {...}}`), or numbers inside `forms.json` / `weapons.json`. The
   first keeps "all numbers in one file" as `CLAUDE.md` says; the second keeps a form in one
   place. M2 starts with weight and strength, so this is needed first.
2. **Should aim assist respect the turn rate?** The tech-test version snaps the character by
   up to `strength × angle` at attack start, which is a turn faster than the turn rate – in
   tension with "turn rate always applies". Alternative: aim assist only biases the direction
   the character turns towards, at its turn rate.
3. **`UE_ROOT` is not set on this machine.** Set it once
   (`setx UE_ROOT "D:\dev\Epic Games\UE_5.8"`), otherwise every script stops with a message.
4. **Frame cap 120 fps** (`Config/DefaultGame.ini`) – fine as a default?
5. **Keep the training dummy** until the M2 AI opponent replaces it?

Session end: 15:45.

### 2026-11-16 – repository set up

- Repository skeleton: agent instructions, roadmap, glossary, Phase 1 design, starting
  values (`data/values.json` v1, taken over from the tech test and adapted to positional
  combat), tech-test findings, Blender pipeline notes. No Unreal project yet – that is the
  first item of M1 and needs a machine with the engine.
