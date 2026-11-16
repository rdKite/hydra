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
| **Directional block** | Protects only an **arc to the front** (the **block arc**); hits inside it are strongly reduced. The size of the arc depends on the shield or weapon. | Stamina per hit (depends on the weight of the attack); no regeneration while blocking; you turn more slowly while blocking | Whoever gets to your flank or back goes around the block |
| **Dodge** | **Pure movement:** a quick step or jump, **without invulnerability**. Only whoever is still inside the hit area according to the server gets hit. | Stamina per step | Short recovery afterwards; whoever jumps too early lands in the next attack |

### Shields and weapons in the block

- **Buckler:** small arc, quick to turn, light – weak block.
- **Round shield:** medium arc.
- **Tower shield:** large arc and very strong block, but very slow to turn while blocking.
- **Blocking with the weapon** (two-handed or without a shield): small arc, weaker than a
  shield. Swords block well, daggers poorly.

### Turn rate

- How fast a character can turn depends on the **ratio weight ÷ strength** (weight = body
  plus load). This is the same logic as for stamina costs and interrupts.
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

- Lower stamina costs (strength and shield weight when blocking, dexterity when dodging).
- More damage reduction.
- Faster turning (through more strength or less load; skills mainly while blocking).
- A slightly wider arc.
- Longer and faster dodge steps – at high levels up to a short-range teleport, for example
  behind the opponent (as a trait).

### Stamina, not openings

- **Rebound:** an attack that hits a block costs the attacker extra stamina – without
  timing, depending on weight.
  - Equal weight: the attacker loses a noticeable amount; the defender pays for the block.
  - Dagger against giant club: the defender pays a lot of stamina, the attacker hardly
    anything.
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

- Stamina does not regenerate while you block.
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
- **Colossus** (e.g. Queen Spider): no blocking or dodging (at most magical). In return: a
  large HP pool, area attacks, slow. This is the one exception to "no meat shields". The
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
  the arc. Dodging counts only if the attack would otherwise have hit you (the server
  checks this via the hit area).
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
- **Deliberately simple:** detection works by comparing two values, not through sight,
  sound and smell channels.
  - **Stealth** (active skill, grows by use): raises the chance of not being detected.
  - **Perception** (attribute, see [progression.md](progression.md)): how well you detect
    hidden enemies.
- **Perception grows independently of level**, and not only through sneaking enemies –
  also by recognising telegraphed attacks and by exploring. So whoever plays in an area
  without sneaking enemies has no lasting disadvantage.
- **Higher perception** lets you recognise telegraphed area attacks earlier and better. The
  telegraph itself stays visible to everyone; perception only gives a head start.
- Race or species sets the base value, cap and growth of perception, as with other
  attributes (wolves, for example, high).
- **Light and dark** (proposal): a dark field makes hiding easier, a light field reveals.
  This follows from the opposition of the two elements.
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
| **What** | Bushes, tall grass, crops and similar block the view; you can walk through them | Icons show enemies nearby when their stealth loses against your own perception |
| **How** | Real geometry – what you cannot see, you cannot see | Comparison of two values (see above) |
| **Accuracy** | Exact as soon as you have a clear view | Rough: direction and approximate position, no silhouette |

A bush does not hide you from the icons, and high perception does not let you see through
bushes – each layer has its own counter.

- **Terrain acts only visually, not on values:** a forest offers more cover by itself than
  a meadow, because more objects break the line of sight there (bushes, trunks,
  undergrowth). A crop field covers well, but rustles when you move. No terrain bonuses on
  the stealth value.
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
| **Weight** (for bows: draw weight) | Stamina per strike, rebound on the block, weak-point scaling | Strength, stamina, rebound by weight |
| **Speed** | Attacks per second | Dexterity |
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
- **Arrowheads** set the damage type: broad for slash, narrow and armour-piercing for
  pierce, blunt for blunt.

### Free choice of hands

- Every weapon has a weight; the grip is free.
  - **Two-handed:** less stamina per strike, more force, stronger weapon block – the off
    hand is occupied.
  - **One-handed:** more stamina per strike, slower – but the off hand is free for a
    shield, focus or a second weapon.
- **The limit follows by itself** from stamina cost ≈ weight ÷ strength × skill factor: if
  a strike costs more stamina than someone has in total, it does not work. With enormous
  strength even two greatswords are possible – expensive, but a nice goal.
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
- **Light weapons scale more strongly** with the weak-point multiplier – the lighter the
  weapon relative to your own strength (weight ÷ strength), the more. So the dagger needs
  no special rule; with light weapons it pays off especially to get to the back.
- This gives a clear spectrum: **light = speed and weak points, heavy = force and stamina
  pressure on blockers** (shield breaker, concussion).
- **Dodging opens the way to the weak point:** a step to the side or behind the opponent –
  especially while they are committed to a missed attack – brings you to flank or back.
  The bonus comes from position, not from a timing window.
- Weak points are recognisable, and you can protect them (turn towards the opponent, neck
  frill). PvE opponents do that too.

### Skills and animations

- **One skill per family** (dagger fighting, swordsmanship, axe fighting, blunt weapons,
  polearms, archery, shield fighting) instead of one per individual weapon. Size is
  handled by strength and weight. Skill titles and traits accordingly (e.g. *Sword
  Master*).
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
- The threshold depends on the **weight** of the target (body plus load): heavy characters
  and colossi are hard to interrupt, light ones easy.
- **Certain weapon types** have properties that shift interrupts in their favour: blunt weapons interrupt
  more easily; heavy weapons are themselves harder to interrupt during their attack.

### What skills improve

No unlocked moves – skills improve values and properties that you can feel:

- more damage;
- less stamina or mana per attack;
- stronger damage increase in chains and a higher cap, a longer window before the chain
  ends;
- faster drawing and switching;
- interrupts: interrupt more easily, or be harder to interrupt yourself;
- depending on the family: wider block arc and faster turning while blocking (sword,
  shield), higher weak-point multiplier (dagger), more reach (spear, bow);
- magic: range, size of element fields, faster build-up of debuffs.
- **Traits** bring a new property at certain levels instead of new moves. Example in
  swordsmanship: the chain no longer ends from a single hit taken. Skill titles give a
  percentage on damage every 30 levels.

### Later

Situational attacks (sprint attack, falling attack, counter after a dodge, throwing the
weapon).
