# Combat

This document holds the full combat design: guiding principles, blocking and dodging
(including turn rate), stealth and perception (including sight cover), physical weapons and
attacks. Terms follow `GLOSSARY.md`. Magic has its own document: [magic.md](magic.md).

**Parry and perfect dodge are dropped** from the design. Combat is **positional combat**:
directional block, rebound, turn rate, telegraphs and lag compensation. There are no timing
windows for perfect actions.

---

## 1. Combat principles

### Skill can beat stats (PvP and PvE)

- Better stats give a noticeable edge. But a good player must always be able to beat an
  otherwise clearly stronger opponent through personal skill.
- This is the principle behind all balancing.
- So stats and augments may have strong effects, **as long as the opponent can react** to
  them (visible, telegraphed). "Playing hard pays off" depends on exactly this.
- Deliberate exception: defense towers (see [world.md](world.md), "Cities, relations and
  zones").

### Positional combat, not timing

- Distance, facing, terrain and managing stamina and mana decide fights – not windows of a
  few milliseconds.
- Reference for positional combat: Path of Titans. The weight of hits stays Souls-like.
- Reasons:
  - fair with different pings (decisions happen over hundreds of milliseconds);
  - fits creatures (a spider does not parry);
  - size differences matter by themselves;
  - no disputes caused by lag.
- Chains, switching weapons and spells, and augments make combat more mobile and varied
  than in a classic Souls game.
- Fights against large monsters are reminiscent of Monster Hunter.
- Hack-and-slash does not fit: endless combos contradict stamina and high damage.
- Every monster form plays noticeably differently.

### Stalking and trading

- Evenly matched opponents circle each other a lot, wait for the right opening and trade
  hits.
- Each side looks for a trade that works out in its favour.
- The choice of element and weapon adds another tactical layer.
- **No pure trading:** whoever stands still in front and trades hits must clearly lose to
  someone who positions well. The tools for this: weak points at flank and back, the
  directional block, high damage and long telegraphs. Otherwise whoever has more health
  simply wins (a common criticism of Path of Titans).

### Cooldowns only for big body skills

- Cooldowns exist only for big body skills (e.g. charge, roar). They are then **visible on
  the character** (stance, glow).
- Everything else follows "resource instead of cooldown": stamina and mana are the limit,
  and both sides see the bars.

### Camera

- Third person, behind or over the shoulder.
- Only this way do size differences (slime to titan), flying and wall running work.
- Weak points on the back require real positioning.

### Aiming

- Soft aim assist plus optional hard **lock-on**.
- Lock-on only turns the camera. Positioning stays important.

### Attacks commit

- An attack that has started plays through.
- The only way to cancel it is the **feint** (costs stamina).
- After the moment of impact, the recovery can be cancelled with a dodge.

### Pace

- Medium: faster than Dark Souls (more like Elden Ring or Lies of P), slower than
  hack-and-slash.
- Stamina sets the rhythm: short series of attacks, then catch your breath.

### Hit feedback

- A short pause on impact (**hit stop**), clear sounds and clear telegraphs – so that high
  damage feels fair.

### Rather no hard stuns

- Locking an opponent down completely, or being locked down yourself, slows the dynamics.
- Slowing would be conceivable.

### High damage, no meat shields

- Attacks deal relatively high damage. Few hits decide a fight – so position, block and
  dodge matter.
- Exception: colossi (see [Monsters: three branches](#monsters-three-branches)).
- Consequences: resistances and armour become more important. Healing in combat is rather
  little and slow.
- Guard rail: against roughly equal opponents, no death from full health in one hit
  without a clear telegraph.
- Deliberate exception: a newcomer who takes on a legendary opponent may fall to one hit.
  In return, the hXP are huge by themselves: an opponent who can kill you with one hit is
  already far stronger by the numbers. No extra bonus is needed.

---

## 2. Blocking and dodging

Principle: **position, not timing windows.** There is no parry and no perfect dodge any
more. Whether a hit lands depends on where you stand and where you face. The server can
check that fairly, even with different pings.

| | How it works | What it costs | Weakness |
|---|---|---|---|
| **Directional block** | Protects only an **arc to the front** (the **block arc**); hits inside it are strongly reduced. The size of the arc depends on the shield or weapon. | Stamina per second while raised (shield weight relative to body weight) plus a share of the **damage actually blocked** per hit; you turn more slowly while blocking | Whoever gets to your flank or back goes around the block |
| **Dodge** | **Pure movement:** a quick step or jump, **without invulnerability**. Only whoever is still inside the hit area according to the server gets hit. A **capability** of the form (see below). | Stamina per step | Short recovery afterwards; whoever jumps too early lands in the next attack |

### Dodge and jump are capabilities

- **Dodge** and **jump** are **capabilities** of the form or race (see
  [monsters.md](monsters.md#capabilities)): passive, they do not level. Whoever has the
  capability can do the action; whoever does not, cannot. Most species have both;
  **gigantic forms have neither** – they are such a large target that dodging would hardly
  help. Their counter is a large HP pool and high resistances.
- **Dexterity** sets how good the action is: dodge distance and speed, jump height (small
  range), fall damage. Dodging trains dexterity; there is no dodge skill.
- **Dodge cost** rises with weight (body plus load) and is set by the form; the exact
  formula comes with balancing.
- **Alternatives replace the standard** (later): e.g. a dodge roll or a short-range teleport
  instead of the dodge step. A form has at most one variant per action.

### Shields and weapons in the block

- **Buckler:** small arc, quick to turn, light – weak block.
- **Round shield:** medium arc.
- **Tower shield:** large arc and very strong block, but very slow to turn while blocking.
- **Shield weight works both ways:** the heavier the shield, the more stamina it costs per
  second to **hold** it raised, but the **less** stamina a blocked hit costs. Balancing must
  keep large shields worthwhile.
- **Blocking with the weapon** (two-handed or without a shield): small arc, weaker than a
  shield. Swords block well, daggers poorly.

### Turn rate

- How fast a character can turn depends on the **ratio weight ÷ strength** (weight = body
  plus load). Strength sets the handling (turning, running speed, drawing a bow); stamina
  costs and interrupts do not depend on it.
- Heavily armed characters and above all gigantic forms turn more slowly. Light and small
  ones react faster.
- This is what makes flanking possible in the first place: whoever runs around a heavy
  opponent reaches flank and back. Against a light opponent you have to do more for it.
- **Slowed further:**
  - while blocking (tower shield the most, see above);
  - during an attack in progress (attacks commit – you can only adjust your facing a
    little, with heavy weapons less than with light ones).
- **Lower and upper limit:**
  - Nobody is helpless. Even a colossus still turns (and covers its back with area attacks
    and burst).
  - Even the lightest forms do not turn instantly; otherwise flanking them would be
    impossible.
- **Lock-on** only turns the camera, not the character any faster. Turn rate always
  applies.
- The turn is visibly animated, so the opponent can read it.
- It runs server-authoritatively (the block arc counts as the server knows the facing).

### What stats and skills give

Each quantity has **one source** (see [progression.md](progression.md#one-quantity-one-source)):

- **Strength:** faster turning (weight ÷ strength), also while blocking.
- **Dexterity:** longer and faster dodge steps, higher jumps.
- **Shield skill:** a slightly wider arc (and traits).
- **Stamina costs** depend only on what you carry relative to your body weight – neither
  stats nor skills lower them.
- A short-range teleport behind the opponent is no longer a trait; it becomes an
  alternative dodge capability.

### Stamina, not openings

- **Block cost and rebound** both come from the **damage actually blocked** – one base, no
  separate weight formula:
  - the blocker pays a share of the blocked damage as stamina (smaller with a heavier
    shield);
  - the attacker loses a share of the blocked damage as stamina (**rebound**) – without
    timing;
  - heavy weapons deal more damage and so press blockers harder by themselves; axes
    (shield breaker) and blunt weapons (concussion) add their property on top.
  - So against heavy opponents, dodging is often the better choice.
- **Whiff:** a missed attack commits the attacker (attacks play through). That is the
  opening for flank and back. No stun.
- Both sides see the stamina on the bar; the opening appears by itself.
- **Dodging always works**, no matter how strong the opponent is. Whoever is out of the
  hit area in time is not hit.

### Long telegraphs

- The wind-up is clearly longer than a typical ping (guide value: heavy attacks at least
  about 400 ms).
- You react to the wind-up and the facing of the opponent, not to the impact.
- In return, fast light attacks deal little damage.

### Against constant blocking and spam

- A raised block costs stamina every second and stamina does not regenerate while you
  block (no regeneration during any action, see [magic.md](magic.md#stamina-and-mana)).
- When stamina is empty, the block breaks: you take the hit, but are not stunned.
- **Feints:** cancelling an attack (with the block key) costs stamina. But it can bait the
  opponent into a block (no regeneration) or a dodge step (costs stamina).
- Attacks are readable: blockable, unblockable and area attacks are clearly telegraphed
  (colour, stance, sound).

### Blocking magical attacks (proposal, still to be refined)

- A block is always a block and costs stamina. What it stops depends on the shield and
  augments. There is no separate "magic block".
- A pure physical tank with a tower shield can also block spells, but only partly: the
  shield stops the bolt, but part of the element damage gets through.
- With a **defensive augment on the shield** (elemental or magic shield), the block also
  stops the magical damage completely. In return, it binds mana. This gives a physical
  tank a reason for some spirit too.
- **Bolts burst on the block; reflecting them: no.** If a bolt hits the block arc, it
  bursts in front of the shield. Its small element field appears there instead of under
  the blocking character's feet.
- Rejected: the earlier "parrying spells" (the explosion hits the caster's allies). It was
  dropped together with the parry.

### Monsters: three branches

- **Agile** (spider, lizard): long dodges or jumps, hardly any blocking.
- **Armored** (beetle, turtle): strong directional block with a wide arc (armour at the
  front), hardly any dodging. Flank and back are the weakness.
- **Colossus** (e.g. Queen Spider): no blocking or dodging (at most magical; no dodge and
  jump capability). In return: a large HP pool, high resistances, area attacks, slow. This is the one exception to "no meat shields". The
  skill here lies in placing area attacks and controlling space.
- Slimes and similar creatures get no defence type of their own, but a high physical
  resistance.
- Evolutions can shift the branch. Humanoids choose their focus through equipment (shield,
  two-handed weapon, two weapons, light armour).

### How it fits together

- Defensive augments act on the block.
- **Resistances:** dodged = no hit, no resistance XP. Block = reduced hit, some resistance
  XP ("dodge or harden").
- **Skill XP:** only actions against real attacks count. Blocking counts on a hit inside
  the arc. Dodging trains **dexterity** (not a skill) and counts only if the attack would
  otherwise have hit you (the server checks this via the hit area).
- **Reserve:** with low reserve, constant blocking becomes even more expensive, and
  positioning matters all the more.

### PvE opponents behave like players

- The same rules for everyone: stamina, mana, augments, block, dodge, feints, the same turn
  rate and telegraphs – no invisible bonuses.
- Opponents should already be challenging at the same level, and all the more above it.
- Build in human mistakes: reaction time and error rate per opponent. Rare opponents fight
  more cleverly (fits the hXP).
- Effort: the AI is the most expensive part. Even the arena (waves) needs it. Start there
  with simple behaviour and make it cleverer step by step.

### Reviving in groups

- Whoever drops to 0 HP in a group is first **downed** instead of dead at once, and can be
  revived by group members. This saves you from surprising overkill attacks – and
  afterwards you know what is coming.
- **Time downed:** rather long, e.g. 60 seconds (common in other games: about 30 to 90
  seconds). You can give up yourself at any time to cut it short.
- **Reviving costs mainly time** (a few seconds) during which the reviver is unprotected.
- **PvP:** downed opponents may be finished off.

### Technology and controls

- Online and server-authoritative: hits are checked via **positions, facing and hit
  areas** with **lag compensation**.
- There are no timing windows for perfect actions – so combat is clearly more tolerant of
  ping.
- Test early in the arena with simulated latency.
- Controller:
  - Block with the key of the hand holding the shield, or with two-handed weapons with the
    second hand key (hold = block; see [Attacks](#5-attacks)).
  - Dodge on an action button.
  - Feint: cancel an attack with the key of the other hand.

---

## 3. Stealth and perception

- **Hard to notice, never invisible** (guiding principle: you must be able to react).
  Close up you can always be seen, in PvP for example as a slight shimmer.
- **Deliberately simple:** detection works by comparing **perception against perception**,
  not through sight, sound and smell channels, and **without chance**. There is no stealth
  skill: whoever perceives well also knows how to stay unnoticed.
- **Detection radius:** two characters with **equal perception** notice each other at a
  default radius (guide value about 15 m, between 10 and 20 m). The **ratio** of the two
  values shifts it – the ratio, not the difference, so the radius between equals stays the
  same at every level. Lower limit (e.g. 3 m: close up you are always noticed), upper limit
  (e.g. 40 m).
- **What shifts the radius** (few, all visible and predictable):
  - **Movement:** standing still < crouching < walking < running/sprinting. **Crouching** is
    a capability (most humanoids and wolves; slimes don't need it, giants can't); it also
    makes the character lower, which works together with sight cover.
  - **Weight including equipment** (noise, tremor): heavy armor and a colossus are noticed
    from further away, a slime from close by. The effect is dampened (e.g. square root or
    logarithm), so a colossus is not noticed from a kilometre away. Light armor makes a
    scout. The magic bag weighs nothing.
  - **Light and dark fields:** a dark field makes hiding easier, a light field reveals.
- **Telegraphs are not affected:** telegraphed area attacks are purely visual and the same
  for everyone; perception only concerns noticing hidden characters.
- **Perception grows** by detecting hidden characters, by staying unnoticed near enemies and
  by exploring – so whoever plays in an area without hidden enemies has no lasting
  disadvantage. Race or species sets its base value (wolves and saurians, for example,
  high).
- **Display:** perceived characters you cannot see are shown **roughly** – direction and
  approximate area – on a minimap or compass (which one is open). The **server** blurs the
  position before sending it, so a cheat tool cannot read the exact one.
- **Attack from hiding:** ends the hidden state. The first hit counts like a weak-point hit
  (even from the front), but is never lethal from full health against roughly equal
  opponents.
- **What it is for:** avoiding fights (especially for monsters that are weaker at first),
  scouting, landing the first blow. Hardly relevant in the arena.
- **Disguise** (can you recognise a transformed monster form?) is related, but a separate
  question.

### Sight cover: bushes, tall grass, crops (as in Path of Titans)

**Two layers, clearly separated:**

| | Sight cover (purely visual) | Perception (value comparison) |
|---|---|---|
| **What** | Bushes, tall grass, crops and similar block the view; you can walk through them | Icons show enemies inside your detection radius (perception against perception) |
| **How** | Real geometry – what you cannot see, you cannot see | Comparison of two values (see above) |
| **Accuracy** | Exact as soon as you have a clear view | Rough: direction and approximate position, no silhouette |

A bush does not hide you from the icons, and high perception does not let you see through
bushes – each layer has its own counter.

- **Terrain acts only visually, not on values:** a forest offers more cover by itself than
  a meadow, because more objects break the line of sight there (bushes, trunks,
  undergrowth). A crop field covers well, but rustles when you move. No terrain bonuses on
  the detection radius.
- **Always shown the same way:** covering plants are game objects, regardless of graphics
  settings and distance. Otherwise whoever hides grass and bushes would see hidden
  characters.
- **Size matters:** a bush covers only up to its height – a slime disappears in the grass,
  a wolf in a bush, a colossus nowhere. Bushes stay naturally sized (not as huge as in
  Fortnite); large forms stick out.
- **Movement gives you away:** whoever moves through a bush or crops makes it sway and
  rustle. Whoever stands still is hidden best.
- **Camera:** the third-person camera must not peek over bushes or around corners while
  the character stays hidden (a known problem in Path of Titans). For example, keep the
  camera low inside a bush; try it out in the arena.
- **Only sight, no protection:** attacks, arrows and spells pass through plants.
- **NPCs:**
  - The server checks by line of sight (raycast). Plants sit in a collision channel that
    blocks sight but not movement (Unreal: AI Perception with its own trace channel;
    Unity: raycast with a layer mask).
  - On top of that, the same value comparison for perception (a wolf finds you even in a
    bush).
  - NPCs can hide the same way.
  - Checks only within a radius and spread over several ticks.
- **Against wallhacks (from Phase 2, see [../roadmap.md](../roadmap.md)):** the server
  sends hidden characters to a client only once that client can see or perceive them – the
  same sight checks as for NPCs.

---

## 4. Weapons (physical)

Magic works without items (see [magic.md](magic.md)). This section is about physical
weapons.

### Properties

| Property | Affects | Related to |
|---|---|---|
| **Weight** (for bows: draw weight) | Stamina per strike (weight ÷ body weight); base damage; whether strength or dexterity scales the damage; handling | Strength (handling), stamina |
| **Demand** | Factor on the stamina per strike on top of the weight – how much force and control the weapon asks for; rises with rarity | Rarity (see [items.md](items.md#what-makes-a-weapon-better)) |
| **Reach** | Distance to the opponent | – |
| **Damage type** | Slash, pierce, blunt | Subtypes of physical |
| **Weapon property** | One special ability per family | Counterpart to the body skills of monsters |

### Families instead of one- and two-handed

| Family | Sizes (light → heavy) | Damage | Weapon property |
|---|---|---|---|
| **Daggers** | Knife, dagger, long dagger | Pierce, slash | **Reverse grip:** blade held pointing down, very close, fast, many stabs; weak weapon block, strong against weak points |
| **Swords** | Short sword, longsword, greatsword | Slash, pierce | All-rounder, good weapon block; the heavy ones with a **wide swing** (several opponents in the arc) |
| **Axes** | Hand axe, battle axe | Slash, blunt | **Shield breaker:** whoever blocks against it loses a lot of stamina |
| **Blunt weapons** | Mace, war hammer | Blunt | **Concussion:** even a normal block costs a great deal of stamina; strong against armour |
| **Polearms** | Spear, maybe halberd | Pierce (spear), plus slash (halberd) | **Reach:** greatest melee reach; a thrust stops a charge (counter against saurians) |
| **Bows** | Short bow, longbow (maybe later composite bow) | Depends on the arrowhead | **Drawing costs stamina** while you aim; counter against flyers |
| **Shields** | Buckler, round shield, tower shield | – | Directional block: buckler small arc, quick to turn, weak block; tower shield large arc, very strong block, slow to turn |

- **Daggers and swords are separate families**, even though both are blades – the fighting
  styles differ too much. Daggers are held in reverse grip, swords normally. Both have a
  size limit: no tiny swords, no large daggers.
- Rejected: fist weapons (no role of their own next to the other weapons; fighting unarmed
  still works, but is not a weapon type) and crossbow (bow subtypes instead).
- **Bows:**
  - Short bow – low draw weight, fast, also while running, little stamina, short range.
  - Longbow – high draw weight (needs strength), long range and high damage, drawing costs
    a lot of stamina, rather while standing.
  - **Strength sets how fast you can draw** a bow, or whether you can draw it at all. A
    bow drawn only briefly deals less damage, flies less far and sways more.
  - **Steady aim as sway, not spread:** the crosshair visibly sways; the arrow flies exactly
    where it points on release. The sway shrinks the longer you draw and the higher your
    **dexterity**; with almost empty stamina it grows. No random spread. The sway must be
    computable from time and a seed, so the server can check the shot.
  - **No aim assist** and no automatic aiming through lock-on for bows – otherwise the sway
    would be pointless.
- **Arrowheads** set the damage type: broad for slash, narrow and armour-piercing for
  pierce, blunt for blunt.

### Free choice of hands

- Every weapon has a weight; the grip is free.
  - **Two-handed:** less stamina per strike, more force, stronger weapon block – the off
    hand is occupied.
  - **One-handed:** more stamina per strike, slower – but the off hand is free for a
    shield, focus or a second weapon.
- **The limit follows by itself** from stamina cost ≈ weapon weight ÷ body weight × demand:
  if a strike costs more stamina than someone has in total, it does not work. Strength sets
  how well a heavy weapon can be handled (turning, drawing); with enormous strength and
  endurance even two greatswords are possible – expensive, but a nice goal.
- **Speed** is not a stat of its own: light weapons are fast because their wind-up is
  short, and that wind-up stays fixed per weapon (telegraphs). Dexterity does not make
  attacks faster.
- Exceptions to this logic: bows always need two hands, shields always one. With light
  weapons the two-handed grip brings hardly anything.
- A weapon held two-handed counts as **one** weapon and binds one set of augments.
- **Switching grip in combat** (proposal): one key, a short moment; a shield moves onto
  the back. A tactical choice between defence and force.

### Damage types: slash, pierce, blunt

- Subtypes of physical, with affinity and imprint like the other elements.
- Blunt helps against armour (armoured saurian), pierce against light armour, slash
  against the unarmoured.
- Example (proposal): slimes shrug off slash and pierce well, but are vulnerable to blunt.

### Weak points instead of random crits

- **No random critical hits** – luck should not decide (guiding principle).
- Instead, **weak points** with a fixed damage multiplier: for most monsters and humanoids
  the back; depending on the species also others (e.g. the flank of the horned saurian,
  whose neck frill protects to the front).
- **Dexterity scales the weak-point multiplier**, independent of the weapon's weight. Light
  weapons scale their damage with dexterity too, so a dexterous fighter seeks the back. The
  dagger's property "strong against weak points" therefore needs a **bonus of its own** on
  the multiplier (set with the weapon numbers).
- This gives a clear spectrum: **light = dexterity, speed and weak points, heavy = strength,
  force and stamina pressure on blockers** (shield breaker, concussion).
- **Dodging opens the way to the weak point:** a step to the side or behind the opponent –
  especially while they are committed to a missed attack – brings you to flank or back.
  The bonus comes from position, not from a timing window.
- Weak points are recognisable, and you can protect them (turn towards the opponent, neck
  frill). PvE opponents do that too.

### Skills and animations

- **One skill per family** (`dagger_fighting`, `sword_fighting`, `axe_fighting`,
  `mace_fighting` for blunt weapons, `polearm_fighting`, `archery`, `shield_fighting`)
  instead of one per individual weapon. Size is handled by strength and weight. Skill
  titles and traits accordingly (e.g. *Sword Master*).
- **Animations** per family, weight class and grip instead of per individual weapon –
  every combination of weapon and grip would otherwise be too expensive.

### For the arena

Longsword with shield, spear, short bow – this covers melee, reach and ranged combat. Magic
without a weapon comes on top.

---

## 5. Attacks

### One key per hand

Each hand has a key (e.g. left and right shoulder button). It does what the hand holds:

| In the hand | Press key | Hold key |
|---|---|---|
| Weapon | Light attack | Heavy attack (charged) |
| Shield | – (free) | Directional block |
| Spell form | Bolt, impact, burst | Sustain stream, or charge bolt |
| Two-handed weapon | Attack (one hand key) | Block with the weapon (other hand key) |

- **Two-handed unlocks special functions**, e.g. precise aiming for magical attacks –
  another advantage of the two-handed grip. Bows are always two-handed and so always have
  aiming.

### Light and heavy

- **Light:** fast, little stamina, little damage.
- **Heavy:** holding the key charges it, more stamina and more damage. The charging is
  visible and telegraphs the attack (like drawing a bow).
- **Weapons with two damage types** split them between light and heavy attack (e.g.
  sword: light slash, heavy pierce; axe: light slash, heavy blunt).
- Rejected: the stick direction sets the damage type.

### Chains instead of rhythm combos

- It is enough to string many attacks together: each further hit in a chain deals more
  damage. No rhythm, no fixed key sequences.
- The limit is set by **stamina and mana**: whoever manages their resources well keeps the
  chain going longer and hits harder and harder.
- **Mixing magic and physical pays off:** whoever mixes weapon attacks (stamina) and spells
  (mana) lasts longer with both resources and keeps the chain going longer.
- **The chain ends** after a short pause without a hit (e.g. two seconds) or when you are
  interrupted yourself. The bonus has a cap.

### Interrupts instead of stuns

- A hit cancels the opponent's attack in progress if its **damage is high enough** – not
  tied to heavy attacks. No stun: the opponent can dodge or block at once; only their
  attack is gone.
- The threshold is a **share of the target's maximum HP**: a single hit must deal at least
  that share. Characters with a large HP pool (colossi) are hard to interrupt by
  themselves; weight, weapon type and size at character creation play no role.

### What skills improve

No unlocked moves – skills improve values and properties that you can feel:

- more damage (added to the pool of progress bonuses, see
  [progression.md](progression.md#damage));
- stronger damage increase in chains and a higher cap, a longer window before the chain
  ends;
- faster drawing and switching;
- interrupts: interrupt more easily, or be harder to interrupt yourself;
- depending on the family: wider block arc (sword, shield), more reach (spear, bow);
- magic: range, size of element fields, faster build-up of debuffs.
- **Not:** lower stamina or mana costs (costs come only from what you use), faster turning
  (strength) or a higher weak-point multiplier (dexterity) – one quantity, one source.
- **Traits** bring a new property at certain levels instead of new moves. Example in
  sword fighting: the chain no longer ends from a single hit taken. Skill titles give a
  percentage on damage every 30 levels.

### Later

Situational attacks (sprint attack, falling attack, counter after a dodge, throwing the
weapon).
