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
  shows up as automatic skill points distributed by use, with a stand-in for the threat
  rating per enemy type – §17.)
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
| **Stamina** | short-term, physical | weapon attacks, dodging, holding and using a block, sprinting | within seconds | endurance (10 stamina per point) |
| **Mana** | short-term, magical | spells | within seconds | spirit (10 mana per point) |

Rules from the attribute model (decision 2026-11-17; formulas in §26):

- **Reserve** (the long-term bar) is **not in Phase 1.** The arena has only stamina and mana.
- You cannot chain attacks or spells endlessly. You must pause to catch your breath.
- **Weapon attack cost** comes from **weapon weight ÷ body weight × the weapon's demand**
  only. Neither strength nor skill lowers it (one quantity, one source). Phase 1 has only
  the lowest rarity, so demand is 1. If one attack costs more than the whole bar, the weapon
  cannot be used – the limit emerges by itself. **Balancing target:** a human at level 0
  manages just under two light strikes in a row with the longsword.
- **Spell cost:** a fixed mana cost per spell form, more for charging.
- **Regeneration** is a share of the maximum per second, **fixed per form**.
- **No regeneration during an action:** stamina does not regenerate while attacking
  (wind-up, strike, recovery), dodging (step and recovery) or holding a block; mana not
  while casting or charging. Walking and running do not count as an action. This replaces
  the fixed regeneration delay.
- A slime has little stamina but a lot of mana (its base values of endurance and spirit).

## 5. Weight

**Weight = body plus load** (carried weapons and shield). Each system has one source
(decision 2026-11-17):

| System | Source |
|---|---|
| Attack cost | weapon weight ÷ **body weight** × demand (§4) |
| Dodge cost | total weight ÷ body weight × the form's dodge cost (§8) |
| Holding a block | shield weight ÷ body weight (§8) |
| Turn rate | total weight ÷ **strength** (§10) – strength sets the handling |
| Strength or dexterity for damage | a fixed share per weapon schematic or body skill, not from weight (§12) |

Rebound, block cost and interrupts no longer depend on weight (§7, §8). The weak-point
multiplier depends on dexterity, not on weapon weight (§11).

Carried weapons count towards weight even when not equipped. In Phase 1 the humanoid
carries only its fixed loadout (longsword and shield).

## 6. Attributes relevant to Phase 1

Decision 2026-11-17 (full model: [progression.md](progression.md#attributes-resistances-and-body-attributes)).

**Body attribute = base value of the form × (1 + 2 % × level)** (+ training, which is not in
Phase 1: the arena keeps no progress). An evolution brings new base values at level 0, so
the new form is weaker at first.

| Attribute | Effect in Phase 1 |
|---|---|
| **Vitality** | 10 HP per point (healing rate: not in Phase 1, there is no healing) |
| **Endurance** | 10 stamina per point – the bar only |
| **Spirit** | 10 mana per point – the bar only |
| **Strength** | handling: turn rate (weight ÷ strength); damage of heavy weapons |
| **Dexterity** | damage of light weapons; weak-point multiplier; dodge distance and speed |
| **Resistance** | points per damage type (slash, pierce, blunt, fire), turned into damage reduction by one flattening curve; no level growth; negative points = weakness |

- Reference: a human at level 0 has 100 vitality and endurance, 60 spirit, strength 80
  (weight units).
- Regeneration is not an attribute: a share per second, fixed per form (§4).
- Skills raise no attributes.
- Perception, jump height and fall damage are not used in Phase 1.

**Capabilities:** dodge and jump are capabilities of the form, not skills – they do not
level. Humanoid and slime have both.

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
- The threshold is a **share of the target's maximum HP** (decision 2026-11-17): a single hit
  must deal at least that share. Weight, weapon type and size play no role.

### What skills improve

No unlocked moves. Skills give **effect, not lower costs** (decision 2026-11-17). Phase 1
builds:

- more damage (added to the pool of progress bonuses, §12);
- shield fighting: a wider block arc;
- fire magic: a larger fire field.

Not by skills: stamina or mana costs (§4), turning (strength), the weak-point multiplier
and the dodge (dexterity). Chain, interrupt and range improvements by skill are in the
design but not built in Phase 1.

A skill level-up must feel different in the hand, not only in a number. (Traits and skill
titles are out of scope.)

## 8. Defence: directional block and dodge

Principle: **position, not timing windows.** There is no parry and no perfect dodge. Whether
a hit lands depends on where you stand and where you face. The server can check that
fairly, even with different pings.

| | How it works | Cost | Weakness |
|---|---|---|---|
| **Directional block** | protects only an **arc to the front**; hits inside it are strongly reduced; arc size depends on shield or weapon | stamina per second while raised (shield weight ÷ body weight) plus a share of the **damage actually blocked** per hit; no regeneration while blocking; slower turning while blocking | flank and back are open |
| **Dodge** | **pure movement:** a quick step or jump, **no invulnerability**; you are hit only if the server says you are still inside the hit area. A **capability** of the form; distance and speed from dexterity | stamina per step (total weight ÷ body weight × the form's dodge cost) | short recovery afterwards; jumping too early lands you in the next attack |

### Block arcs

- Shields in Phase 1: one shield (the notes describe buckler = small arc, round shield =
  medium arc, tower shield = large arc). Which shield type Phase 1 uses: **round shield
  (assumption)**.
- **Block with the weapon** (two-handed or without shield): small arc, weaker than a shield.
  Swords block well. (Not needed for the Phase 1 loadout.)
- Arc angles and damage reduction: TBD (tune in playtests).

### Rebound

- Block cost and rebound both come from the **damage actually blocked** (decision
  2026-11-17), no timing and no separate weight formula:
  - the blocker pays a share of the blocked damage as stamina – smaller with a heavier
    shield (which in turn costs more per second to hold);
  - the attacker loses a share of the blocked damage as stamina (**rebound**).
- Heavy attacks deal more damage and so press a blocker harder by themselves. Against heavy
  enemies dodging is often the better choice.

### Misses bind the attacker

- A missed attack still plays out. That is the opening for flank and back. No stun.
- Both sides see stamina on the bars; openings arise by themselves.
- **Dodging always works**, however strong the opponent: if you are out of the hit area in
  time, you are not hit.

### Anti-turtling

- A raised block costs stamina every second, and stamina does not regenerate while blocking.
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

- Turn rate depends on **weight ÷ strength** (weight = body plus load). Strength sets the
  handling; stamina costs and interrupts do not depend on it. Heavily armed and large forms turn slower; light and small
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
- **Dexterity scales the multiplier**, independent of the weapon's weight (decision
  2026-11-17). Light weapons also take their damage from dexterity (§12), so a dexterous
  fighter seeks the back.
- Spectrum: light = dexterity, speed and weak points; heavy = strength, force and stamina
  pressure on blockers.
- **Dodging opens the way:** a step to the side or behind the enemy – especially while it is
  bound in a missed attack – reaches flank or back. The bonus comes from position, not a
  timing window.
- Weak points are recognisable and can be protected (turn to face the enemy). AI enemies do
  this too.
- Multiplier and dexterity scaling: TBD (tune in playtests).

## 12. Damage and hit feedback

- **High damage, no meat shields.** Attacks deal relatively high damage; a few hits decide.
  That is why position, block and dodge matter. Consequences: resistances matter more;
  healing in combat is rare and slow.
- **Guardrail:** against roughly equal opponents, no death in one hit from full health
  without a clear telegraph. **No hard cap** (decision 2026-11-19): it is a balancing
  target – the strongest **light** hit stays below the HP of an equal opponent. Phase 1 has
  no armor, so a telegraphed, charged heavy hit to the back may fell a character – the price
  of bad position.
- **Hit stop:** a short pause on hit, clear sounds (placeholders), clear telegraphs – so high
  damage feels fair.
- **Damage types:** physical splits into slash, pierce and blunt. Elements in Phase 1: fire
  only. Proposal from the notes: slimes shrug off slash and pierce well but are vulnerable
  to blunt. (Phase 1 has no blunt weapon.)
- **Damage formula** (decision 2026-11-17, formulas in §26): base damage of the weapon,
  body attack or spell form × (1 + **progress bonuses added up**: skill, strength or
  dexterity, imprint for the element part) × **situational factors multiplied**: charge,
  chain, weak point – each capped. Then each damage type is reduced by the target's
  resistance curve. **No direct damage or HP bonus per level** – the level acts through the
  attributes.
- Strength or dexterity: a **fixed share per schematic or body skill** (decision 2026-11-19;
  longsword 50/50, slam 70/30); bonus = strength share × strength bonus + dexterity share ×
  dexterity bonus. Both count only what the level added on top of the form's base value.
- **Base damage follows from the cost** (decision 2026-11-19): light base damage =
  efficiency × cost of the light strike, efficiency = 0.41 × (weight ÷ 1.5 kg)^0.1 –
  longsword light **200**. A heavy attack deals × 2 at × 1.6 cost; full charge × 1.5 on top.

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
| Swords | **longsword** (middle of short sword → longsword → greatsword), **1.5 kg**, strength / dexterity 50 / 50 | slash (light), pierce (heavy) | all-rounder, good weapon block |
| Shields | **shield** (buckler / round shield / tower shield) | – | directional block; arc and block strength by shield type |

- Hands: a shield always takes one hand. The longsword is used one-handed here (one-handed =
  more stamina per attack, slower; the free off-hand holds the shield).
- Skills: one skill per weapon family. Phase 1 uses **sword fighting** and **shield
  fighting**. Dodge is a capability, not a skill (decision 2026-11-17). (The notes also list "block" as a separate skill; Phase 1 treats
  blocking with a shield as shield fighting – assumption.)
- Weapon properties (data): weight, demand (1 at the lowest rarity), speed, reach, damage
  type(s), weapon property.
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
- Capabilities: dodge and jump, like the humanoid.

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
skills (by use), attributes (from the form and the level in Phase 1, §6). Phase 1 needs the
following.

### Character level

- Grows through **nXP** from defeated enemies. nXP depends only on enemy type and level,
  not on your own level.
- Each level raises the body attributes by 2 % of the form's base value (§6): more HP,
  stamina and mana, better handling, more damage through strength or dexterity. There is no
  direct HP or damage bonus per level.
- The level sets **when** skills can rise and when milestones come.
- XP curve and nXP per enemy: TBD (tune in playtests).

### Skill growth: hXP and automatic skill points

Decision 2026-11-19 (full design: [progression.md](progression.md#skill-points-automatic-and-free)).

- A defeated enemy gives **nXP** (character level) and **hXP**. In Phase 1 there is no
  threat rating yet; a **stand-in factor per enemy type** sets hXP = nXP ×
  `enemies.<id>.hxp_factor` (at least 1).
- **hXP are distributed by use:** each skill gets a share in proportion to the stamina and
  mana its actions spent in the fight (weapon attacks → weapon skill; holding a block and
  blocked hits → shield fighting; slam → slam; bolt → projectile, a fire bolt also fire
  magic). Movement and dodging belong to no skill.
- **Only on victory, per enemy** (decision 2026-11-16): hXP are credited when an enemy is
  defeated; a player who is defeated loses only what is pending since the last enemy
  defeat.
- **At each character level-up** the character gets **automatic skill points**: number =
  1 + 2 × log₂(hardship rating), hardship rating = hXP ÷ nXP since the last level-up (at
  least 1). They are split in proportion to the hXP each skill got, **with fractions**: each
  skill has a progress bar (0–1); whole points become skill levels, the rest stays.
- **Limit:** a skill's level is at most the character level (form level). A share above it
  goes to the other used skills, or waits for the next level-up.
- Not in Phase 1: free skill points, renown and rXP, teachers, the learning phase (it only
  touches weapon families at level 0, which Phase 1 does not have), respawn penalty.

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
- Effects: resistance points and a damage bonus (progress bonus, element part) for that
  element, element tint on body skills (slime
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
    bonus** on that skill and **relearns it faster** (its share of the hXP counts more). Strength depends on how
    high the skill was relative to the old form's cap. If the new form lacks the skill, the
    instinct is kept for later.
- Instinct bonus size: TBD (tune in playtests).

### Arena rules

- **Start at fixed levels** (e.g. shortly before a level cap) so evolution can be tested at
  once. Start levels per form: `forms.<id>.start_level` (see §25).
- No lasting progress and **no respawn penalty** in the arena.

*Not in Phase 1:* threat rating (a stand-in factor per enemy type instead), free skill
points, renown and rXP, teachers, the learning phase, skill titles and traits, respawn penalty, multiple imprints, physical imprint, rarity-gated forms. Full design: [progression.md](progression.md).

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
- Threat rating (stand-in factor per enemy type), free skill points, renown, rXP, teachers,
  Adventurers' Guild; matchmaking, groups, reviving, contribution overview.
- Attribute training by strain; resistance growth by exposure; healing rate.
- Perception and the detection radius; crouch; jump height and fall damage from dexterity.
- Rarity and demand above 1.
- Menus beyond the bare minimum; real sound (placeholders only); Steam Deck.
- Everything of later phases: world, survival, factions, strategy layer, MMO,
  monetization.

## 22. Open values

Needed by Phase 1, not set in the notes – all TBD (tune in playtests):

- base attributes per form (vitality, endurance, spirit, strength, dexterity);
  regeneration share per form; cost per action;
- block arc angles, block damage reduction, holding cost, share of blocked damage for
  blocker and rebound;
- dodge distance and cost, recovery, dexterity scaling;
- turn rate floor and cap, slowdown while blocking and attacking;
- interrupt threshold as a share of maximum HP;
- chain bonus per hit, chain cap;
- weak-point multiplier, dexterity scaling;
- weapon damage, size of each progress bonus, share "heavy" and its limits, resistance
  curve constant;
- fire bolt cost and damage, field size and duration, burning damage per stack, stack cap;
- elemental damage that goes through a plain shield;
- element-body damage of the fire elemental slime;
- XP curves, imprint thresholds, instinct size, start levels;
- wave composition, AI reaction time and error rate.

## 23. Rules as built in M2 (starting formulas)

> **M4b replaces parts of §23–§25** with the attribute model of §26: attack and dodge cost,
> regeneration delay, interrupt, block cost, rebound, weak point, resistance, the level
> bonuses, what a skill level does, the dodge skill, skill XP and skill levels
> (now hXP and automatic skill points). The build session updates these tables
> once M4b is built.

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

## 26. Attribute model for Phase 1 (M4b, decision 2026-11-17)

What M4b builds, so that the M5 playtest tests the rules that stay. Formulas and starting
numbers are **guesses** for `data/values.json` (bump the version, mark them as guesses in
`data/CHANGELOG.md`); the build session may choose different numbers if a live run shows
they do not work, and logs why. Full design: [progression.md](progression.md),
[combat.md](combat.md), [magic.md](magic.md).

**Attribute gain.** For each body attribute: value = `base × (1 + level_growth × level)`
(`level_growth` = 0.02). The **gain** g = value ÷ base − 1 (in Phase 1 = 0.02 × level, no
training) is the one number that strength and dexterity bonuses scale with.

| Rule | Phase 1 |
|---|---|
| Base attributes | per form in `forms.<id>`: vitality, endurance, spirit, strength (weight units), dexterity, plus body weight. Guide values: humanoid 100 / 100 / 60 / 80 / 100; slime and fire elemental slime keep their current proportions to the humanoid (slime ≈ 60 / 50 / 100 / 12, fire elemental slime ≈ 80 / 60 / 130 / 16), dexterity as the build session sees fit |
| Bars | HP = 10 × vitality, stamina = 10 × endurance, mana = 10 × spirit. A humanoid without a spell form shows no mana bar |
| Regeneration | `forms.<id>.stamina_regen_share_per_s` and `mana_regen_share_per_s` × maximum. None while an action runs (attack from press to end of recovery, bolt from press until it is out, dodge step and recovery, held block). Replaces `stamina.regen_delay_after_action_ms` |
| Attack cost | `attack.light/heavy_stamina_per_weight_ratio × weapon weight ÷ body weight × weapons.<id>.demand` (demand 1); body attacks (slam) use `weapons.<id>.cost_ratio` in place of weight ÷ body weight. Longsword `weight` 1.5 kg (real weapon, decision 2026-11-19); one-handed (grip factor 1.0); the strength requirement per grip is not built (the longsword needs 60 one-handed, the humanoid has 80). Slam `cost_ratio` ≈ 0.01 (guess: about two slams in a row for a slime at level 0, like the longsword for a human; see `monsters.md`). Balancing target: a human at **level 0** manages just under two light longsword strikes in a row from a full bar (at the arena start level 9 the bar is 18 % larger – a little over two) |
| Dodge cost | `forms.<id>.dodge_stamina_cost × total weight ÷ body weight` (strength plays no role) |
| Dodge | distance × (1 + `dodge.distance_per_dexterity_gain` × g_dex), duration ÷ (1 + `dodge.speed_per_dexterity_gain` × g_dex) – small effects |
| Holding a block | `block.hold_stamina_per_s_per_weight_ratio × shield weight ÷ body weight` per second while raised |
| Blocked hit | blocked damage = incoming damage × `block_damage_reduction`. Blocker pays blocked damage × `block.blocker_share` × `block.reference_shield_ratio ÷ (shield weight ÷ body weight)` (limited to `share_min … share_max`); attacker loses blocked damage × `block.rebound_share` (no rebound for bolts). Block break as before |
| Interrupt | a hit cancels a binding attack if its damage (after block and resistance) ≥ `interrupt.threshold_share_of_max_hp` × target's maximum HP |
| Damage | `base damage × (1 + skill bonus + attribute bonus + imprint bonus) × charge × chain × weak point`, then per damage type through the resistance curve |
| Skill bonus | `skills.damage_bonus_per_level` × level of each skill the action trains (weapon skill; projectile; for a fire bolt also fire magic) – added, not multiplied. Instinct levels count as skill levels |
| Attribute bonus | bonus = s × `damage.strength_bonus_per_gain` × g_str + (1 − s) × `damage.dexterity_bonus_per_gain` × g_dex, with the fixed strength share s = `weapons.<id>.strength_share` (longsword 0.5, slime slam 0.7). Both bonuses 0.3 per gain (guess). The bolt uses none (spell damage comes from skill and imprint) |
| Base damage | from the cost (decision 2026-11-19): light damage = `damage.efficiency_base` (0.41) × (weight ÷ `damage.efficiency_reference_kg` (1.5))^`damage.efficiency_exponent` (0.1) × light cost of a human at level 0 (body 70 kg, 1,000 stamina, one-handed). Longsword light **200**; heavy × 2 (cost × 1.6), full charge × 1.5. The slam: the same efficiency at its own light cost for a slime at level 0 (about 95). Store the resulting base damage in `weapons.<id>` (data, not computed at runtime) |
| Imprint bonus | `imprint.damage_bonus_per_level` × imprint level, on the fire part only |
| Situational factors | charge and chain as before (each capped); weak point: `weak_point.damage_multiplier + weak_point.multiplier_per_dexterity_gain × g_dex`, capped at `max_damage_multiplier` |
| Resistance | points per type in `forms.<id>.resistance` (+ `imprint.resistance_points_per_level` × imprint level for fire). Reduction = p ÷ (p + `resistance.curve_constant`) for p ≥ 0 (constant 100: 100 points = −50 %); for p < 0 damage × (1 + \|p\| ÷ constant). Current shares convert: 0.5 → 100 points, −0.25 → −25 points. Burning interval ÷ (1 − reduction) as before |
| Element body | `element_body_damage` × (1 + imprint bonus), fire, through the attacker's resistance (no level factor any more) |
| Removed | `progression.hp_bonus_per_level`, `progression.damage_bonus_per_level`, `skills.cost_reduction_per_level`, `skills.min_cost_factor`, `skills.xp_per_dodge`, `skills.dodge_distance_bonus_per_level`, the dodge skill in `skills.json` and `forms.json`, the weight-based interrupt and block keys, `weak_point.reference_weapon_weight_ratio`. "Dodges that avoided a hit" stays as a metric |
| Scale | HP, stamina and mana grow about tenfold. Weapon and slam damage follow from their cost (row above: a human at level 0 falls after five light longsword hits, as in M4). Scale bolt, burning and element-body damage and the mana costs so that a fight lasts about as long as in M4 |

**Skill growth (decision 2026-11-19, §17).**

| Rule | Phase 1 |
|---|---|
| hXP | a defeated enemy gives hXP = its nXP × `enemies.<id>.hxp_factor` (≥ 1; stand-in for the threat rating). Guesses: grunt 1.0, veteran 1.5, ember slime 1.5. Every player in the arena gets the full nXP and hXP |
| Use per skill | the server sums per player and skill the stamina and mana spent since the last enemy defeat: weapon attack cost → weapon skill; block holding cost and blocker's stamina for blocked hits → shield fighting; slam → slam; bolt mana → projectile, and for a fire bolt the same amount also to fire magic. Dodge costs count for no skill |
| Credit | when an enemy is defeated, its hXP are split over the skills in proportion to that use and added to each skill's pending hXP for the current level, together with its nXP. A defeated player loses only the use counted since the last enemy defeat. No use at all: the hXP go to no skill but still count for the hardship rating |
| Automatic skill points | at each level-up: points = `skill_points.base` + `skill_points.factor` × log₂(max(1, hXP ÷ nXP of this level)) (1 and 2). Split in proportion to each skill's hXP of this level, times (1 + `instinct.xp_bonus_max` × instinct ratio); fractions go into a progress bar per skill (0–1), whole points raise the level. A skill at the character level takes no more; its share goes to the other skills used this level, else waits as progress. Then the counters of the level start again |
| Removed | `skills.xp_base`, `xp_per_level`, `xp_per_hit`, `xp_per_block`, `xp_per_bolt_hit` (and `xp_per_dodge`, see above); the rule "at most one level per level-up and skill" |
| HUD | per skill its level and progress bar; the hardship rating of the current level |

Unchanged: turn rate (weight ÷ strength, now with the grown strength), imprint progress and
milestones, evolution and instinct (except its XP bonus, see above), waves, metrics, lag
compensation.

## 27. Debug panel (M5 playtest kit, decision 2026-11-19)

A test tool, not a game menu. Built after M4b, so "set skill" works on the new skill
growth.

- **Toggle:** F1 (gamepad: a free button, e.g. Back/View). Built in C++ (Slate or a HUD
  menu drawn by the HUD class), no Blueprint logic; usable with mouse, keyboard and gamepad.
  While open, game input is paused for the own character only (the game keeps running).
- **Own character:**
  - **form:** every form in `forms.json` (humanoid, slime, fire elemental slime; new forms
    appear by themselves);
  - **level:** set directly; attributes, bars and the skill limit follow at once;
  - **skills:** set each skill of the form to a level (and its progress bar), at most the
    level as the rule says; set the fire imprint level; open a milestone;
  - **reset:** full bars, debuffs removed.
- **Enemies:** spawn any enemy type from `enemies.json` a few metres in front of the own
  character, with a chosen level and active or passive; remove all enemies; waves on/off.
- **Server-authoritative:** the panel only sends requests; the server applies them, also
  for the client window.
- **Every action is also a console command** (e.g. `HydraSetForm`, `HydraSetLevel`,
  `HydraSetSkill <skill id> <level>`, `HydraSetImprint <level>`, `HydraSpawn <enemy id>
  <level> [passive]`, `HydraClearEnemies`), so live runs can use and test the same path.
  Existing commands (`HydraForm`, `HydraXp`, …) are folded in or kept as aliases; README
  updated.
- **Development builds only:** compiled out of shipping builds (`UE_BUILD_SHIPPING`).
- Verified by a live run that drives the commands on host and client and checks the
  result on both.
