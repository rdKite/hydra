# Phase 1 design – arena prototype

This document holds the game rules that Phase 1 needs. It is written for the people and
agents who implement them. It covers only what Phase 1 builds. The full design (later
phases, other lines, world, strategy layer) is in the other files of `docs/design/` (start
at `overview.md`).

Terms follow `GLOSSARY.md`. **Parry and perfect dodge are dropped** from the design. The
most important combat terms:

| German | English | Meaning |
|---|---|---|
| Richtungsblock | **directional block** | a block that protects only an arc to the front (the **block arc**) |
| Rückprall | **rebound** | extra stamina the attacker loses when a hit lands in a block |
| Drehgeschwindigkeit | **turn rate** | how fast a character can turn |
| Ansage / Ausholen | **telegraph** / **wind-up** | the readable lead-in before an attack lands |
| Positionskampf | **positional combat** | fights decided by distance, facing and terrain, not timing windows |
| Verzögerungsausgleich | **lag compensation** | the server rewinds to what the attacker saw when it checks a hit |

**Numbers.** Where the notes give example numbers, they appear here as *starting values
(tunable)*. Where a value is needed but the notes give none, it says **TBD (tune in
playtests)** here; `data/values.json` may hold a first guess for it, marked as such in
`data/CHANGELOG.md`. All numbers live in `data/values.json`; none are hard-coded.

---

## 1. Goal of Phase 1

Phase 1 answers one question: **Are combat and progression fun, and not too complicated?**

- Runs locally in **host mode** (listen server plus a second client window). Client and
  server are separated from the start, so multiplayer later does not need a rewrite.
- **Humanoid and slime** are both playable. The humanoid tests positional combat with
  weapons. The slime tests evolution and the core of the monster game.
- Every milestone (M1–M5, see `docs/roadmap.md`) ends with something playable.
- Rough estimate: 4–6 weeks of regular work (uncertain: animation and tuning cost more than
  pure logic).

## 2. Guiding principles

These rules shape implementation decisions everywhere.

- **Skill can beat stats (PvP and PvE).** Better stats give a noticeable edge. But a good
  player must always be able to beat a clearly stronger opponent through personal skill.
  Stats may have strong effects **as long as the opponent can react** to them: they are
  visible and telegraphed. Short form: *you can react to everything.*
- **Playing hard pays off.** Harder fights give more skill growth relative to character XP.
  Two characters at the same level can be very different in strength. (In Phase 1 this
  shows up only as "skills grow by use"; hXP is out of scope.)
- **Positional combat, not timing.** Distance, facing, terrain and managing stamina and
  mana decide fights – not windows of a few milliseconds. Reference for the positional feel:
  Path of Titans. Weight of hits: Souls-like. Reasons:
  - fair with different pings (decisions happen over hundreds of milliseconds);
  - fits creatures (a spider does not parry);
  - size differences matter by themselves;
  - no disputes caused by lag.
- **Stalking and trading.** Evenly matched opponents circle each other, wait for an opening
  and trade hits. Each side looks for a trade that favours it.
- **No pure trading.** Whoever stands still in front and trades hits must clearly lose to
  someone who positions well – through weak points at flank and back, the directional
  block, high damage and long telegraphs. Otherwise whoever has more HP simply wins.
- **Resource, not cooldown.** Stamina and mana limit actions; both sides see the bars.
  Cooldowns exist only for large body skills and must then be visible on the character.
  Phase 1 has no such skill, so Phase 1 has no cooldowns.
- **No hard stuns.** Nobody gets locked in place. Slowing is acceptable.
- **No random crits.** Luck must not decide.
- **Pace:** medium – faster than Dark Souls (closer to Elden Ring or Lies of P), slower than
  hack-and-slash. Stamina sets the rhythm: short bursts of attacks, then catch your breath.
- **Every monster form plays noticeably differently.**

## 3. Authority and data

- **Server-authoritative.** Every gameplay decision is made on the server, also in host
  mode: hits, damage, stamina, mana, block, interrupts, debuffs, XP, level-ups, imprints and
  evolution. Clients predict only what the engine predicts well (movement, ability
  activation).
- **Data-driven.** Forms, skills, elements, weapons and all numbers are data files with
  stable IDs. They are reloadable at runtime without recompiling. This is the basis for
  balancing and later mods. **All numbers are in `data/values.json`** – general rules as
  sections, the numbers of one form, weapon or shield in the sections `forms`, `weapons`
  and `shields`, keyed by ID (decision 2026-11-16). `forms.json`, `skills.json`,
  `elements.json` and `weapons.json` say what exists and how it relates.

## 4. Resources

Phase 1 has two bars: **stamina** and **mana**.

| Bar | What | Drained by | Regenerates | Governed by |
|---|---|---|---|---|
| **Stamina** | short-term, physical | weapon attacks, dodging, blocking, sprinting | within seconds | endurance |
| **Mana** | short-term, magical | spells | within seconds | spirit |

- **Reserve** (the long-term bar) is **not in Phase 1.** The arena has only stamina and mana.
- You cannot chain attacks or spells endlessly. You must pause to catch your breath.
- **Weapon attack cost.** A sword attack costs 10–90 % of the stamina bar depending on
  weapon weight, strength and skill (*starting values, tunable*). Rough formula:

  `stamina cost ≈ weapon weight ÷ strength × skill factor`

  A higher skill lowers the cost noticeably (per sword attack, per dodge). If one attack
  costs more than the whole bar, the weapon cannot be used – the limit emerges by itself.
- **Spell cost.** A spell costs mana in the same way (exact cost: TBD (tune in playtests)).
- **Blocking stops stamina regeneration** for as long as the block is held.
- **Species sets base value, cap and growth.** A slime has little stamina but a lot of mana.
- Regeneration rate, regeneration delay and bar sizes: TBD (tune in playtests).

## 5. Weight

**Weight = body plus load** (carried weapons and shield). One rule drives several systems:

| System | Effect of weight ÷ strength |
|---|---|
| Movement | moving (running, dodging) costs stamina by weight relative to strength; heavier = more costly and slower |
| Attack cost | heavier weapon = more stamina per attack |
| Rebound | heavier attack = more stamina drained from a blocker; see §8 |
| Interrupt | heavier target = harder to interrupt; see §6 |
| Turn rate | heavier relative to strength = slower turning; see §10 |
| Weak points | lighter weapon relative to strength = bigger weak-point bonus; see §11 |

Carried weapons count towards weight even when not equipped. In Phase 1 the humanoid
carries only its fixed loadout (longsword and shield).

## 6. Attributes relevant to Phase 1

Attributes are level-independent and capped. In the full design they grow through strain
and exposure. In the arena there is no lasting progress, so **in Phase 1 attributes are
fixed values per form** from the data files (assumption; growth by strain is out of scope).

| Attribute | Effect in Phase 1 |
|---|---|
| **Strength** | divisor in weight ÷ strength: attack cost, turn rate, weak-point scaling; damage of heavy weapons; lower block cost (with shield weight) |
| **Endurance** | stamina maximum and regeneration |
| **Dexterity** | attack speed, precision; lower dodge cost |
| **Vitality** | maximum HP |
| **Spirit** | mana maximum and regeneration |
| **Resistance** | per element and physical; reduces damage of that type (the slime line has raised physical resistance) |

Perception is not used in Phase 1 (stealth is out of scope).

---

## 7. Combat basics

### Camera and aiming

- Third-person camera behind or over the shoulder. Size differences (slime vs. humanoid)
  and weak points at the back need real positioning.
- **Soft aim assist** plus optional **lock-on**. Lock-on **turns only the camera**, never
  the character faster. Turn rate always applies – also to aim assist: at the start of an
  attack it gives the character a facing part of the way towards a target in front, and the
  character turns there at its turn rate (decision 2026-11-16).
- **Facing while moving** (decision 2026-11-16): normally the character turns into its
  movement. With **precise movement** (a toggle; keyboard X, gamepad left stick click) it
  keeps facing where the camera looks and steps backwards and sideways, at the strafe
  speed. A **raised block** always faces the camera direction, so the block arc can be
  aimed without lock-on; the same is planned for drawing a bow. Turn rate applies to all
  of it. In precise movement there is no aim assist (the player aims with the camera).
- Controller and mouse/keyboard.

### Attacks: one button per hand

Each hand has one button (e.g. left and right shoulder button). It does what the hand holds:

| In the hand | Press | Hold |
|---|---|---|
| Weapon | light attack | heavy attack (charged) |
| Shield | – (free) | directional block |
| Spell form (bolt) | cast bolt | charge bolt |

Dodge is on its own action button (keyboard: Left Alt), jump on another (Space). A jump is
ordinary movement: no stamina cost for now, not possible while an attack binds, while
blocking or during a dodge.

- **Light attack:** fast, little stamina, little damage.
- **Heavy attack:** holding the button charges it; more stamina, more damage. The charge is
  visible and telegraphs the attack.
- **Weapons with two damage types** split them over light and heavy. Longsword: light =
  **slash**, heavy = **pierce**.

### Attacks bind

- A started attack plays out. (The **feint** – cancelling it for stamina, §8 – is out of the
  plan for now, decision 2026-11-16.)
- After the hit moment, the recovery can be cancelled with a dodge.

### Chains

- Chain attacks freely. Each further hit in a chain does more damage. No rhythm, no fixed
  button sequences.
- Stamina and mana are the limit. Mixing weapon attacks (stamina) and spells (mana) keeps a
  chain going longer.
- The chain **ends** after a short pause without a hit (*starting value: 2 seconds,
  tunable*) or when you are interrupted yourself.
- The chain bonus has a cap. Bonus per hit and cap: TBD (tune in playtests).

### Interrupt instead of stun

- A hit cancels the target's running attack if its **damage is high enough** – not tied to
  heavy attacks.
- No stun: the interrupted character can dodge or block at once; only its attack is gone.
- The threshold depends on the **target's weight** (body plus load). Heavy characters are
  hard to interrupt, light ones easy.
- Heavy weapons are harder to interrupt during their own attack.
- Threshold formula: TBD (tune in playtests).

### What skills improve

No unlocked moves. Skills improve values you can feel:

- more damage;
- less stamina or mana per attack;
- stronger chain scaling, higher chain cap, longer window before the chain ends;
- interrupting more easily, being interrupted less easily;
- per family: wider block arc and faster turning while blocking (sword, shield);
- dodge: longer and faster dodge step, less stamina;
- magic: range, element field size, faster debuff build-up.

A skill level-up must feel different in the hand, not only in a number. (Traits and skill
titles are out of scope.)

## 8. Defence: directional block and dodge

Principle: **position, not timing windows.** There is no parry and no perfect dodge. Whether
a hit lands depends on where you stand and where you face. The server can check that
fairly, even with different pings.

| | How it works | Cost | Weakness |
|---|---|---|---|
| **Directional block** | protects only an **arc to the front**; hits inside it are strongly reduced; arc size depends on shield or weapon | stamina per blocked hit (by the attack's weight); no stamina regeneration while blocking; slower turning while blocking | flank and back are open |
| **Dodge** | **pure movement:** a quick step or jump, **no invulnerability**; you are hit only if the server says you are still inside the hit area | stamina per step | short recovery afterwards; jumping too early lands you in the next attack |

### Block arcs

- Shields in Phase 1: one shield (the notes describe buckler = small arc, round shield =
  medium arc, tower shield = large arc). Which shield type Phase 1 uses: **round shield
  (assumption)**.
- **Block with the weapon** (two-handed or without shield): small arc, weaker than a shield.
  Swords block well. (Not needed for the Phase 1 loadout.)
- Arc angles and damage reduction: TBD (tune in playtests).

### Rebound

- A hit that lands in a block costs the **attacker** extra stamina – no timing, based on
  weight.
- Equal weight: the attacker loses noticeably, the defender pays for the block.
- Dagger against giant club (the club attacks, the dagger wielder blocks): the defender pays
  a lot of stamina, the attacker hardly anything. Against heavy enemies dodging is often the better choice.

### Misses bind the attacker

- A missed attack still plays out. That is the opening for flank and back. No stun.
- Both sides see stamina on the bars; openings arise by themselves.
- **Dodging always works**, however strong the opponent: if you are out of the hit area in
  time, you are not hit.

### Anti-turtling

- No stamina regeneration while blocking.
- When stamina is empty, the **block breaks**: you take the hit but are not stunned.
- **Feints** (out of the plan for now, decision 2026-11-16): cancelling your own attack (with
  the block button, i.e. the other hand's button) costs stamina. It can bait the opponent into a block (no regeneration) or a dodge
  (costs stamina).

### Projectiles against a block

- A projectile hitting the block arc **bursts in front of the shield**. Its small element
  field appears there, not under the blocker's feet. No reflecting.
- A plain shield stops the projectile, but part of the elemental damage goes through.
  (Defensive augments that stop all of it are out of scope.) Share that goes through: TBD
  (tune in playtests).

## 9. Telegraphs

- The wind-up is clearly longer than a typical ping. *Starting value: heavy attacks at least
  about 400 ms (tunable).* You react to the wind-up and the opponent's facing, not to the
  impact.
- Fast light attacks deal little damage in return.
- Blockable, unblockable and area attacks are clearly marked (colour, stance, sound).
  Phase 1 sound is placeholder only.

## 10. Turn rate

- Turn rate depends on **weight ÷ strength** (weight = body plus load) – the same logic as
  stamina cost and interrupt. Heavily armed and large forms turn slower; light and small
  ones react faster.
- This makes flanking possible: circling a heavy enemy gets you to flank and back; against a
  light enemy you must work harder.
- **Extra slowdown:**
  - while **blocking** (the bigger the shield, the stronger);
  - during a **running attack** (attacks bind – little re-aiming; heavy weapons less than
    light ones).
- **Floor and cap:** nobody is helpless – even the slowest character still turns. And even
  the lightest form does not turn instantly, or flanking it would be impossible.
- **Lock-on does not speed up turning.**
- Turning is visibly animated so the opponent can read it. It runs server-authoritatively:
  the block arc counts as the server knows the facing.
- Phase 1 check: humanoid with shield vs. slime must show a clear difference.
- Floor, cap, slowdown factors: TBD (tune in playtests).

## 11. Weak points

- **No random critical hits.**
- Instead: **weak points** with a fixed damage multiplier. For most monsters and humanoids
  this is the **back**. (Other species can have others; none in Phase 1.)
- **Lighter weapons scale more** with the weak-point multiplier: the lighter the weapon
  relative to your strength (weight ÷ strength), the bigger the bonus.
- Spectrum: light = speed and weak points; heavy = force and stamina pressure on blockers.
- **Dodging opens the way:** a step to the side or behind the enemy – especially while it is
  bound in a missed attack – reaches flank or back. The bonus comes from position, not a
  timing window.
- Weak points are recognisable and can be protected (turn to face the enemy). AI enemies do
  this too.
- Multiplier and light-weapon scaling: TBD (tune in playtests).

## 12. Damage and hit feedback

- **High damage, no meat shields.** Attacks deal relatively high damage; a few hits decide.
  That is why position, block and dodge matter. Consequences: resistances matter more;
  healing in combat is rare and slow.
- **Guardrail:** against roughly equal opponents, no death in one hit from full health
  without a clear telegraph.
- **Hit stop:** a short pause on hit, clear sounds (placeholders), clear telegraphs – so high
  damage feels fair.
- **Damage types:** physical splits into slash, pierce and blunt. Elements in Phase 1: fire
  only. Proposal from the notes: slimes shrug off slash and pierce well but are vulnerable
  to blunt. (Phase 1 has no blunt weapon.)
- HP values, base damage per weapon, chain and level scaling: TBD (tune in playtests).

## 13. Hit detection with lag compensation

- Hits are checked on the server from **positions, facing and hit areas**, with lag
  compensation: the server rewinds to what the attacker saw.
- Rewind covers **melee hits and the block arc** (was the defender facing the attack at that
  moment?).
- Dodge success is decided the same way: still inside the hit area or not.
- There are no timing windows for perfect actions, so combat tolerates ping well.
- **Build it early (M2)** and test it with simulated latency; retrofitting it later would be
  a rebuild. Neither engine has it built in. (The tech test used switchable latency of 0, 50,
  100 and 150 ms plus some packet loss.)

---

## 14. Humanoid: longsword and shield

The Phase 1 humanoid has a fixed loadout: **longsword in one hand, shield in the other.**

| Family | Item | Damage | Weapon property |
|---|---|---|---|
| Swords | **longsword** (middle of short sword → longsword → greatsword) | slash (light), pierce (heavy) | all-rounder, good weapon block |
| Shields | **shield** (buckler / round shield / tower shield) | – | directional block; arc and block strength by shield type |

- Hands: a shield always takes one hand. The longsword is used one-handed here (one-handed =
  more stamina per attack, slower; the free off-hand holds the shield).
- Skills: one skill per weapon family. Phase 1 uses **sword fighting** and **shield
  fighting**, plus **dodge**. (The notes also list "block" as a separate skill; Phase 1 treats
  blocking with a shield as shield fighting – assumption.)
- Weapon properties (data): weight, speed, reach, damage type(s), weapon property.
- Values: TBD (tune in playtests).

## 15. Slime

### Slime (starting form)

- **Very weak:** very low base values.
- **Whole line:** raised resistance to physical damage (soft body). The slime has no block of
  its own; its defence is this resistance.
- **No affinity:** every element is equally easy.
- Little stamina, a lot of mana.
- **Attacks:** a **slam** (melee) and a **projectile** (body skill, base skill of the slime).
  Both start without element. With an element imprint, the projectile takes on that element.
- No rubber jump.
- Rarity: common. **Level cap: 30** (confirmed – the slime is meant to be the weakest monster).
- The slime can dodge (pure movement) like everyone else (assumption: the notes give it no
  defence type, but dodge is a general rule).

### Fire elemental slime (evolution)

| Form | Rarity | Level cap | Unlocked by | Takes along | Property |
|---|---|---|---|---|---|
| **Slime** | common | 30 | start | – | very weak, physically resistant; base skill projectile |
| **Fire elemental slime** | common | 50 | first element imprint (fire) | the highest imprint, reset to level 1 | **element body:** melee hits against it deal some fire damage back to the attacker |

- Keeps the raised physical resistance.
- Its projectile carries fire.
- Size of the element-body damage: TBD (tune in playtests).
- All other slime forms (chimera, prism, primordial, giant, king slime) are out of scope.

### Slime costs

- Which bar the slam and projectile use is not stated. **Assumption:** slam costs stamina
  (physical attack), projectile costs mana (M3 pairs it with mana; slimes have a lot of
  mana). Values: TBD (tune in playtests).

## 16. Magic minimum

Phase 1 has one element (**fire**), one spell form (**bolt**), the **fire element field** and
the **burning** debuff.

**Decision (2026-11-16): in Phase 1 only the slime casts** (through its body). The humanoid
holds longsword and shield and stays purely physical; casting with a free hand comes with
the hand wheels in Phase 1b.

### Rules that apply

- **Mana for spells, stamina for movement.** Casters must dodge too.
- **Casting needs a free hand** (humanoids). Spells sit "in the hand" like weapons.
  **Monsters** cast through the body.
- **Element skill (fire magic)** is the skill of casting. Spell forms unlock by element-skill
  level; the bolt is available from **level 1**.
- **Aiming:** no hard target lock; soft aim assist on controller.

### Fire bolt

| Effect | Cost | Skill | Counter |
|---|---|---|---|
| ball or arrow, like a bow shot; leaves a **small fire field** on impact | fixed mana per shot; charging (hold) makes it stronger and costlier | aiming, leading the target | block (bursts in front of the shield), dodge |

- A fire bolt hit can add burning stacks directly.

### Fire element field

- Bolt impacts leave a fire field on the ground.
- Standing in it applies **burning**.
- Field size and duration: TBD (tune in playtests).

### Burning debuff (stacking)

- While you stand in the field, the debuff's **stack** rises at fixed intervals. The
  interval is **set per element** (*starting value: every 10 seconds, tunable*).
- Higher stack = stronger effect, up to a cap.
- Hits (e.g. a fire bolt) can add stacks.
- After leaving the field, the stack drops the same way: **one stack per interval**.
  Burning keeps burning after you leave.
- **Resistance, affinity and imprint lengthen the interval** between stacks.
- The stack is shown on the debuff icon.
- No hard stun, even at the highest stack.
- Burning = damage over time. Damage per stack and stack cap: TBD (tune in playtests).

---

## 17. Progression minimum

Three separate pillars grow differently: character level (nXP from defeated enemies),
skills (by use), attributes (fixed in Phase 1, §6). Phase 1 needs the following.

### Character level

- Grows through **nXP** from defeated enemies. nXP depends only on enemy type and level,
  not on your own level.
- Each level gives a little more HP and base damage. Most of a character's strength comes
  from skills.
- The level sets **when** skills can rise and when milestones come.
- XP curve and nXP per enemy: TBD (tune in playtests).

### Skill XP by use

- Skills level by use, not by skill points. Each skill collects its own skill XP.
- Skill XP is credited **at the end of a fight, only on victory.** In waves the fight is
  each **single enemy** (decision 2026-11-16): skill XP earned is credited when an enemy is
  defeated; a player who is defeated loses only what is still pending since the last enemy
  defeat – not the whole wave.
- Only actions against real attacks count: blocking counts on a hit inside the arc; dodging
  counts only if the attack would otherwise have hit (server checks the hit area).
- On each **character level-up**, every skill with enough skill XP rises – **at most one
  level per level-up and skill**.
- Until the form's level cap, skills never exceed the current form level.
- Surplus skill XP does not expire; it waits for the next level-up.
- Skill XP needed per skill level: TBD (tune in playtests).

### Imprint (one)

- An imprint is an acquired alignment to an element, levels 1–5.
- **Progress comes from contact with the element**, not from skill XP: dealing and taking
  that element's damage, standing in its field.
- **Milestones every 10 levels** (monsters also at each evolution): there you may start an
  imprint (if progress is enough) or raise one by a level. Only what you have earned is
  offered.
- The first imprint level is clearly easier to reach than later ones.
- **Awakening:** the first imprint level awakens the element: the element skill starts at
  level 1 and the bolt becomes available.
- Effects: resistance and damage for that element, element tint on body skills (slime
  projectile turns fire). The imprint is visible on the character.
- Phase 1 uses **fire** only. Imprint progress thresholds and bonus sizes: TBD (tune in
  playtests).

### Evolution step with instinct

- Evolution happens at a milestone (every 10 levels) if the requirements are met, at the
  latest at the form's level cap. The player chooses body form and imprint together.
- Slime → fire elemental slime requires the **first fire imprint** (§15).
- After evolution:
  - **level resets to 0**; base values reset to the level but are higher in the new form;
  - **skills and imprints reset**; the fire elemental slime keeps its fire imprint at
    level 1 (form-specific bonus);
  - **instinct:** for every skill developed before, the new form gets a **small permanent
    bonus** on that skill and **relearns it faster** (more skill XP). Strength depends on how
    high the skill was relative to the old form's cap. If the new form lacks the skill, the
    instinct is kept for later.
- Instinct bonus size: TBD (tune in playtests).

### Arena rules

- **Start at fixed levels** (e.g. shortly before a level cap) so evolution can be tested at
  once. Start levels per form: `forms.<id>.start_level` (see §25).
- No lasting progress and **no respawn penalty** in the arena.

*Not in Phase 1:* hXP and threat rating, hardship rating, skill titles and traits, veteran
levels, respawn penalty, multiple imprints, physical imprint, rarity-gated forms. Full design: [progression.md](progression.md).

---

## 18. AI enemies and waves

- AI enemies follow **the same rules as players:** stamina, mana, block, dodge, feints,
  same turn rate and same telegraphs. No invisible bonuses.
- **Human mistakes are built in:** **reaction time** and **error rate** per enemy.
- Enemies should be challenging at equal level, more so above it.
- AI is the most expensive part. Start with simple behaviour and make it smarter step by
  step.
- **Some wave enemies deal fire damage** or leave fire fields – that is the slime's path to
  its fire imprint in the arena (decision 2026-11-16).
- M2: one simple AI opponent for practice. M4: enemies in **waves**.
- Enemies protect their weak points (turn to face the player).
- Enemy roster, wave size and wave progression: TBD (tune in playtests).

## 19. Metrics

Collect, for later numbers:

- hits;
- blocked hits;
- dodge steps;
- stamina use.

Output format: CSV (per `docs/roadmap.md`).

## 20. Done when

You can play **10–15 minutes** against waves as a humanoid or as a slime, **level up or
evolve once** on the way, and the metrics deliver first numbers.

## 21. Out of scope for Phase 1

- **Feints** (taken out of the plan on 2026-11-16; to be revisited).
- Online play over real servers; source-built engine; dedicated servers.
- **Reserve** (long-term bar).
- Hand wheels, magic wheel and **augments** (bound mana); switching weapons or grip in combat.
- More weapons (spear, short bow, others) and more elements (ice and the rest); opposing
  elements cancelling; other spell forms (stream, impact, burst).
- Wolf and saurian as playable forms. The wolf pup appears only as a 3D pipeline test
  (model, rig, a few animations), not playable.
- Other slime forms and the physical branch.
- Bushes, sight cover, stealth and perception (Phase 2).
- Armour and crafting; rarity of items.
- hXP, threat rating, matchmaking, groups, reviving, contribution overview.
- Attribute growth by strain; resistance growth by exposure.
- Menus beyond the bare minimum; real sound (placeholders only); Steam Deck.
- Everything of later phases: world, survival, factions, strategy layer, MMO,
  monetization.

## 22. Open values

Needed by Phase 1, not set in the notes – all TBD (tune in playtests):

- stamina and mana: maxima, regeneration, regeneration delay, cost per action;
- block arc angles, block damage reduction, block stamina cost, rebound amount;
- dodge distance, dodge cost, recovery;
- turn rate floor and cap, slowdown while blocking and attacking;
- interrupt threshold by weight;
- chain bonus per hit, chain cap;
- weak-point multiplier, light-weapon scaling;
- HP, weapon damage, level scaling;
- fire bolt cost and damage, field size and duration, burning damage per stack, stack cap;
- elemental damage that goes through a plain shield;
- element-body damage of the fire elemental slime;
- XP curves, imprint thresholds, instinct size, start levels;
- wave composition, AI reaction time and error rate.

## 23. Rules as built in M2 (starting formulas)

The sections above leave several formulas open ("TBD"). M2 needed them, so these are the
formulas in the game now. They are **assumptions to be confirmed or changed in playtests**;
all numbers are in `data/values.json` (see `data/CHANGELOG.md` v2). "Weight ratio" = weight
÷ strength; weight = body plus load.

| Rule | Formula (values.json section) |
|---|---|
| Unit | body weight, weapon weight and strength share one unit; at weight ratio 1 the rules below apply unchanged |
| Turn rate | `base ÷ ratio^exponent`, capped; × `blocking_factor` while blocking, × `attacking_factor` while an attack binds; never below the floor (`turn_rate`) |
| Attack cost | `light/heavy_stamina_per_weight_ratio × weapon weight ÷ strength` (`attack`); skill factor comes with M4 |
| Dodge cost | `dodge.stamina_cost × weight ratio` of the character |
| Heavy attack | every attack starts as a light one and pays the light cost. If the button is still held when the light wind-up is over and the extra stamina is there, it becomes a heavy attack (pays the difference), strikes on release – not before `min_windup_ms` – or at `max_charge_ms`; damage grows linearly to × `full_charge_damage_factor` |
| Chain | damage × (1 + `damage_bonus_per_hit` × hits so far, capped at `max_bonus`); ends `reset_after_ms` after the last hit or when the attacker is interrupted (`chain`). Blocked hits get the multiplier but do not advance the chain |
| Interrupt | a hit cancels the target's attack while it binds (wind-up, charge, strike) if damage ≥ `damage_threshold_per_weight × (target weight + attacking_weapon_weight_factor × its weapon weight)` (`interrupt`). No stun |
| Block cost | `stamina_cost_per_hit × r × (heavy_attack_factor for heavy attacks)`, r = attacking weapon weight ÷ shield weight, limited to `weight_ratio_min … max` (`block`) |
| Rebound | attacker loses `rebound_attacker_stamina_ratio × its attack cost ÷ r` |
| Block break | if the blocker cannot pay the block cost: its stamina goes to 0, the block ends, the hit lands in full |
| Weak point | attacker inside `back_half_arc_deg` around the target's back: damage × `1 + (damage_multiplier − 1) × reference_weapon_weight_ratio ÷ (weapon weight ÷ strength)`, capped at `max_damage_multiplier` (`weak_point`) |
| Hit area | target's body (capsule radius) reaches into `reach_m` and its centre is inside `half_arc_deg` of the attacker's facing; checked every server tick during `hit_ms`, each target once |
| Lag compensation | targets are checked where they were `min(attacker's ping, max_rewind_ms)` ago: position (hit area), facing and block state (block arc, weak point). The attacker itself is taken at the server's present |
| Bind | from the press to the end of the strike: no dodge, no block, slow turning, movement × `attack.move_speed_ratio`; in the recovery a dodge may cancel |
| Dodge | a step of `distance_m` over `duration_ms` in the input direction (backwards without input), then `recovery_ms` without actions; no invulnerability |

Not built in M2 (not on the roadmap list): feints (out of the plan, see §21), block with the weapon, stamina cost for
running, hit stop as more than a short freeze of the pose.

## 24. Rules as built in M3 (slime and magic, starting formulas)

Like §23: assumptions to be confirmed or changed in playtests; numbers in
`data/values.json` (`data/CHANGELOG.md` v4).

| Rule | As built |
|---|---|
| Choosing a form | arena start option per player (`?Form=slime`, see README); no menu yet |
| Size | every form has a body radius and height (`forms.<id>`); hit areas and movement use them |
| Resistance | damage × (1 − resistance of the target for that type); types: slash, pierce, blunt, fire (`forms.<id>.resistance`). The slime resists slash and pierce and is vulnerable to blunt |
| Slime buttons | weapon-hand button = slam (press light, hold heavy, like a weapon); off-hand button = the bolt (press, hold = charge). No block. Dodge and jump like everyone |
| Slam | a body attack with its own numbers (`weapons.slime_slam`), blunt, costs stamina by its weight like a weapon; chain, interrupt, weak point as for weapons |
| Bolt | costs mana: the base cost at the press, the extra for the charge at the release (cut down to what the mana pays for). Wind-up, then it leaves; held longer it charges up to `max_charge_ms`: damage and cost grow linearly to their full-charge factors. Binds like an attack until it is out |
| Aiming | the cast is aimed where the camera looks; the caster turns there **at its turn rate** during the wind-up and the bolt leaves along the facing it has reached. It flies level and straight at `bolt.speed_m_s` up to `bolt.range_m` (no gravity yet) |
| Element | without an element the bolt deals blunt damage and nothing else. With fire awakened (M4 earns that; M3 has a start option and a dev command) it deals fire damage, adds `burning.stacks_per_hit` and leaves a fire field where it lands |
| Bolt against a block | it bursts on the shield: `block.projectile_damage_through` of the damage gets through, the blocker pays `block.stamina_cost_per_hit`, no burning stack, and the fire field appears in front of the blocker (one body radius plus one field radius towards the caster). Empty stamina: block break, full hit |
| Fire field | a circle of `fire_field.radius_m` for `duration_ms`; it burns everyone in it, the caster too |
| Burning | in a field: first stack at once, then +1 per `stack_interval_ms`; outside −1 per interval (it keeps burning after leaving). The interval is divided by (1 − fire resistance). Damage: `damage_per_s_per_stack` × stacks, in ticks of `tick_interval_ms`, as fire damage |
| Mana | per form (`mana_max`, `mana_regen_per_s`); regenerates without a delay; the humanoid has none |

## 25. Rules as built in M4 (progression and waves, starting formulas)

Like §23 and §24: assumptions to be confirmed or changed in playtests; numbers in
`data/values.json` (`data/CHANGELOG.md` v5).

| Rule | As built |
|---|---|
| Start | every player starts at `forms.<id>.start_level` with every skill of the form at `start_skill_level` (humanoid 9 / 6, slime 29 / 20); nothing is kept between sessions |
| Character level | nXP for level L → L + 1 = `xp_base + xp_per_level × L` (`progression`). Per level: maximum HP × (1 + `hp_bonus_per_level` × level), damage of weapon, body attack, bolt and element body × (1 + `damage_bonus_per_level` × level). No level above the form's `level_cap`; nXP at the cap is lost |
| nXP | an enemy gives `nxp_base × (1 + nxp_bonus_per_enemy_level × its level)` – by its type and level only. **Every player in the arena gets it in full** (assumption; groups and sharing are out of scope) |
| Skills of a form | from `forms.json`; plus the element skill (fire magic) once the element is awakened |
| Skill XP | per landed, unblocked weapon or body attack (`xp_per_hit`, to the weapon's skill), per hit taken on the block (`xp_per_block`, shield skill), per dodge that avoided a hit (`xp_per_dodge`), per bolt that hit unblocked (`xp_per_bolt_hit`, to projectile and, for a fire bolt, also to fire magic). Earned XP is **pending** until the next enemy is defeated, then credited; a player who is defeated loses only what is pending since then ("only on victory", per enemy – decision 2026-11-16) |
| Dodge counts | only against a real attack: during the strike the server finds the target outside the hit area, the target started a dodge within the last `dodge.duration_ms + recovery_ms`, and the place where the step began was inside the hit area. Once per attack. (Melee only; bolts do not count yet) |
| Skill levels | at each character level-up every skill whose credited XP reaches `xp_base + xp_per_level × skill level` rises by one (at most one per level-up and skill, never above the character level); the rest of the XP waits. The last enemy of a wave credits first and gives its nXP second, so that level-up can already raise skills |
| What a skill level does | × (1 + `damage_bonus_per_level` × level) damage and × max(`min_cost_factor`, 1 − `cost_reduction_per_level` × level) cost for the skill's actions (weapon skill: attacks; shield skill: cost of a blocked hit; dodge: the step; projectile: the bolt's damage and mana). Also: block arc + `block_arc_deg_per_level` per level (shield), dodge step × (1 + `dodge_distance_bonus_per_level` × level), fire field radius × (1 + `field_radius_bonus_per_level` × fire magic level) |
| Imprint progress | from contact with fire: fire damage taken × `progress_per_damage_taken` plus fire damage dealt × `progress_per_damage_dealt` (bolt, burning, element body). Progress is kept up to what the next level needs (`progress_first_level`, later `progress_per_further_level`) |
| Milestone | reaching a level divisible by `milestone_every_levels`, or the level cap, opens a milestone. **It stays open until it is used** (assumption: so the imprint can still be earned after the level was reached). The HUD offers only what was earned |
| Imprint | at an open milestone with enough progress: the imprint starts or rises by one (key R), the milestone is used, progress starts again. Per level: fire resistance + `resistance_per_level`, fire damage × (1 + `damage_bonus_per_level` × level). Level 1 awakens fire: fire magic starts at level 1, a bolt carries fire |
| Evolution | offered (key E) if the form has an evolution (`forms.json`), a milestone is open **or the level cap is reached**, and the new form's imprint is there or earned (it is then taken together with the evolution). Slime → fire elemental slime |
| After the evolution | level 0, nXP 0, every skill level 0 and skill XP 0, milestones used; the fire elemental slime keeps the fire imprint at level 1 (fire magic 1); full bars with the base values of the new form |
| Instinct | per skill the ratio old skill level ÷ old level cap is kept (the highest ever reached). The skill counts as `skill_level_bonus_max × ratio` levels higher for good, and earns × (1 + `xp_bonus_max × ratio`) skill XP. Skills the new form lacks keep their ratio for later |
| Fire elemental slime | own numbers in `forms.fire_elemental_slime`; shown with the slime's model, tinted and glowing. **Element body:** an unblockable `element_body_damage` × level factor of fire damage back to whoever lands a melee hit on it (after the target's fire resistance) |
| Enemy types | `enemies.json` (form, imprint) with `enemies.<id>` in `values.json`: level, skill level (all skills of the form), nXP and the AI numbers – reaction time, error rate, pauses and chances per type. They are ordinary characters: same stamina, mana, turn rate, telegraphs, level and skill rules |
| Enemy behaviour | as the practice opponent of M2. New: a form with the bolt and `cast_range_m` > 0 stands at 60 % of that range while it has the mana and casts where its target stands (no leading – walking sideways avoids the bolt); without mana it comes close and slams |
| Sides | AI characters do not hurt each other with attacks and bolts (assumption; their fire fields burn everyone). Players can hurt each other |
| Waves | `waves.list` in order, `first_delay_ms` after the start, `break_ms` after a wave is cleared; enemies appear at spots near the arena border. After the last wave the list starts again with every enemy `level_bonus_per_loop` levels higher. A defeated enemy lies for `character.respawn_delay_ms` and is removed; a defeated player respawns as before, the wave goes on |
| Metrics | on the server, one CSV per session (`Saved/Metrics/metrics_<time>.csv`): at every cleared wave and at the end of the session one row per player and one per enemy type with what happened since the last row – attacks, hits, hits that were blocked, misses, hits taken, blocks, block breaks, dodges, dodges that avoided a hit, casts, bolt hits, damage dealt and taken, stamina and mana used, deaths, kills |

Not built in M4: a menu for the milestone choice (two keys and a HUD line instead), enemies
that feint (feints are out of the plan), wave enemies with other weapons or of the plain
slime form, a pause or an end screen, navigation around obstacles (enemies walk straight at
their target).

Not built in M3: gravity or drop for the bolt, aim assist for the bolt, the bolt's own lag
compensation (it is simulated on the server; what the caster sees is the replicated
bolt), mesh tints or effects for burning (text on the HUD only), per-form movement speeds.
