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
  stable IDs (`values.json` and similar). They are reloadable at runtime without
  recompiling. This is the basis for balancing and later mods.

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
  the character faster. Turn rate always applies.
- Controller and mouse/keyboard.

### Attacks: one button per hand

Each hand has one button (e.g. left and right shoulder button). It does what the hand holds:

| In the hand | Press | Hold |
|---|---|---|
| Weapon | light attack | heavy attack (charged) |
| Shield | – (free) | directional block |
| Spell form (bolt) | cast bolt | charge bolt |

Dodge is on its own action button.

- **Light attack:** fast, little stamina, little damage.
- **Heavy attack:** holding the button charges it; more stamina, more damage. The charge is
  visible and telegraphs the attack.
- **Weapons with two damage types** split them over light and heavy. Longsword: light =
  **slash**, heavy = **pierce**.

### Attacks bind

- A started attack plays out. The only way to cancel it is a **feint** (costs stamina, §8).
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
- **Feints:** cancelling your own attack (with the block button, i.e. the other hand's
  button) costs stamina. It can bait the opponent into a block (no regeneration) or a dodge
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
- Skill XP is credited **at the end of a fight, only on victory.** In waves: credited at the
  end of each wave (assumption).
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
  once. Exact start levels per form: TBD (tune in playtests).
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
