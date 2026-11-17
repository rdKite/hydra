# Progression

This document holds the full progression design: character level, skills, attributes,
affinity and imprint, the two kinds of XP, levelling up, evolution, skill titles and traits,
veteran levels, milestones, the respawn penalty, and how XP is shared in group fights.
Terms follow `GLOSSARY.md`. Numbers are guideline values unless stated otherwise; all
gameplay numbers live in `data/values.json`.

---

## Level, skills and attributes

Three separate pillars that grow in different ways:

| | What | Grows through | Level-dependent | hXP |
|---|---|---|---|---|
| **Character level** | base level | nXP from defeated enemies | – | – |
| **Skills** (active) | what you *do* – attack and defence alike (sword fighting, fire magic, shield fighting, weapon smithing) | using them | yes | yes |
| **Attributes** | what you *are* – resistances and body attributes | body attributes: level (+2 % of the base per level) plus strain; resistances: exposure only | body attributes: partly | no |

### Skills

- Skills level up by use, not through skill points. Each skill collects its own skill XP.
  Skill XP is credited at the end of a fight – only on a win. At every character level-up
  the game checks which skills have enough skill XP to level up as well (details in
  [Levelling up](#levelling-up-level-skills-imprint)).
- So letting friends beat you up only pays off if the other person is defeated at the end
  and accepts a respawn penalty.
- **Skills must have a noticeable effect.** Whoever fights a lot with the sword hits
  noticeably harder and keeps chains going longer. A skill level-up should be felt in the
  hand, not only in a number. Skills give **effect, not lower costs** (see
  [One quantity, one source](#one-quantity-one-source)).
- **Not every ability is a skill:** dodge, jump and crouch are **capabilities** of the form
  or race – they do not level (see [monsters.md](monsters.md#capabilities)); how well you
  dodge comes from dexterity. Sneaking is not a skill either (perception against
  perception).

#### Skill list

| Group | Skills (IDs) |
|---|---|
| **Weapon families** (7) | `dagger_fighting`, `sword_fighting`, `axe_fighting`, `mace_fighting` (blunt weapons), `polearm_fighting`, `archery`, `shield_fighting` |
| **Elements** (8) | fire, ice, lightning, earth, light, dark, poison and wind magic – the ability to cast; unlocks the spell forms and covers the elemental augments of that element |
| **Augment** (1) | body augment |
| **Crafting** (6) | `weapon_smithing`, `armor_smithing`, `weapon_carving`, `armor_carving`, `leatherworking`, `tailoring` (see [items.md](items.md#crafting-skills)) |
| **Body skills** (monsters only) | active ones from the shared catalogue: slam, projectile, bite, charge, pounce, howl/roar, tail strike, stomp, threads/web (see [monsters.md](monsters.md#body-skills-monsters)) |

- A humanoid has 22 skills; monsters add their body skills and use no weapon skills.
- Gathering (ore, wood) has no skill – it trains attributes. Building at the camp has none
  for now.
- **What skills may improve:** damage (into the pool of progress bonuses), chains, drawing
  and switching, interrupts, properties of the family (block arc, reach; for magic range,
  field size, debuff build-up), plus traits and titles. **Not:** stamina or mana costs,
  turn rate (strength), weak-point multiplier (dexterity).

### Attributes: resistances and body attributes

Attributes are what you *are*: six **body attributes** and one **resistance** per damage
type. Body attributes grow with the level and through strain; resistances only through
exposure.

#### Value of a body attribute

**Attribute = base value of the species or form × (1 + 2 % × level) + training**

- **Base value:** comes from the body – the form for monsters, the race for humanoids. An
  evolution changes it. Reference: an untrained human at level 0 has 100 in vitality and
  endurance and 60 in spirit; strength stays in weight units (human 80).
- **Level:** +2 % of the base value per level, linear. A human has 100 vitality at level 0
  and 300 at level 100.
- **Training** comes on top: strain close to your own limit, counted per action (a sword
  strike trains strength, separately from the skill XP). Growth depends on intensity –
  only strain that really challenges you relative to your own value counts. Training is
  capped; cap and speed depend on the species (affinity) and are set in balancing.
- **After an evolution** a monster is deliberately weaker at first (new form, level 0) and
  overtakes its old values over time.
- **Neither skills nor traits raise attributes.** Skill titles may passively raise maximum
  HP or the maximum reserve (see [Skills](#skills)).
- **hXP do not flow into attributes.** The hardship system rewards skills and rare
  evolutions.

#### What each attribute does

| Attribute | Gives | Grows through (training) |
|---|---|---|
| **Vitality** | **10 HP per point**; healing rate (always active, also in combat); size of the reserve | surviving heavy damage, healing injuries, holding out long on low reserve |
| **Endurance** | **10 stamina per point** – the bar only | running, sprinting, swimming, flying, fighting until stamina is really empty, carrying heavy armor |
| **Spirit** | **10 mana per point** – the bar only (spell damage comes from element skill, imprint and damage bonuses) | using mana until exhausted |
| **Strength** | damage of **heavy** weapons; handling: turn rate, running and sprinting speed and sprint cost when the weight ratio (weight ÷ strength) is high, how fast a bow can be drawn; whether a heavy weapon can be handled at all | swinging heavy weapons, carrying heavy loads, mining ore, chopping wood |
| **Dexterity** | damage of **light** weapons; the weak-point multiplier; dodge distance and speed; jump height (small range); fall damage; steady aim with a bow (less sway) | dodging, jumping, surviving falls, hits with a bow, precise crafting work |
| **Perception** | detection radius against other characters – perception against perception (see stealth and perception in [combat.md](combat.md#3-stealth-and-perception)) | detecting hidden characters, staying unnoticed near enemies, exploring |

- Endurance, spirit and vitality form a triangle with the three bars stamina, mana and
  reserve (see Stamina, mana and reserve in [magic.md](magic.md)). The **regeneration** of
  all three bars is a share of the maximum per second, **fixed per species**.
- **Dexterity does not shorten wind-ups** (that would break "you can react to everything")
  and gives no "precision" – there are no random crits to make precise.
- **Climbing and balancing** are not part of the design for now.

#### One quantity, one source

When several levers act on the same quantity, they multiply and run away (e.g. a bigger
stamina bar × more strength × skill cost reduction gave nine times as many strikes per bar
at level 100). So each quantity has **exactly one source**, and no hard caps are needed:

| Quantity | Only source |
|---|---|
| Size of a bar (HP, stamina, mana, reserve) | vitality, endurance, spirit (level + training; titles may add HP and reserve) |
| Regeneration | a share per second, fixed per species |
| Cost of an action | what you use and how heavy it is relative to your **body weight**; for weapons also their **demand**; for spells the spell form and charge |
| Damage | see [Damage](#damage) |
| Handling (turning, running and sprinting speed, drawing a bow) | strength (weight ÷ strength) |
| Movement (dodge distance and speed, jump, fall damage) | dexterity |

- **Growth goes into damage, not into more strikes:** a bigger bar lets you use heavier and
  better weapons (more damage, more cost) and stronger spell forms. Strength decides
  *what* you can wield, endurance *how long*.
- **Skills give effect, not lower costs** – also for augments (more effect, not less bound
  mana).

#### Resistances

- **Points, not shares:** a resistance is a point value per damage type. All sources add
  points: base value of the species (from its **affinity**), training through exposure,
  **imprint**, **armor**, an active **defensive augment**.
- **One flattening curve** turns the sum into damage reduction at the very end, e.g.
  `reduction = points ÷ (points + 100)`: 100 points = −50 %, 300 = −75 %, 900 = −90 %. No hard
  cap – every point helps, but less and less; 100 % is never reached.
- **Negative points** are a weakness (more damage), e.g. slimes against blunt.
- **Training only through exposure**, no growth with the level – that is the identity of
  resistances (ice bathing, glaciers):
  - Hits in combat count as external influence and give resistance XP directly on every
    hit – much faster than through the environment. This applies to hits from monsters
    (NPCs); for hits from players see Friendly fire and reputation in [world.md](world.md).
  - The environment also works (staying near fire for a long time, ice bathing), but only
    with a massive time investment. Whoever deliberately spends many hours on it may be
    rewarded – but has to take care not to die doing it.
  - **Growth depends on intensity:** as cold resistance grows, a cold lake gives less and
    less; then you need glaciers in the high mountains. So early grinding is limited by the
    world instead of by the level, and whoever wants more has to explore. Conditions count
    only if they actually take effect (warm armor → no cold).
- **Skills do not shift resistances** (the imprint does that: it describes the being).
  Elemental augments shift nothing permanently.
- **Opposing elements** create trade-offs: many points in one element lower the points in its
  opposite. Three pairs and three loners (poison, wind, physical).

  | Element | Opposite | Raise through environment |
  |---|---|---|
  | **Fire** | ice | lava, desert heat, near fire |
  | **Ice** | fire | ice bathing, glaciers, blizzard |
  | **Lightning** | earth | thunderstorms on mountain peaks, waters with electric eels, charged crystals |
  | **Earth** | lightning | rockfall, sandstorm, earthquakes (earth "grounds" lightning) |
  | **Light** (life) | dark | holy places, glaring salt deserts |
  | **Dark** (death) | light | cursed places, deep caves without light |
  | **Poison** | – | eating poisonous plants in small doses (like mithridatism), swamps, venomous animals |
  | **Wind** | – | storms, mountain ridges, cliffs |
  | **Physical** | – | every hit with weapons or claws; split into **slash, pierce, blunt** (see Weapons in [combat.md](combat.md)) |

  All elements except physical count as magical damage types.
- **Immunity** exists only as an explicit property of a form, not by stacking points:
  **incorporeal beings** (e.g. ghosts) are immune to physical damage but weak against all
  magical damage types; legendary forms may break rules.
- Highly specialised characters remain possible – with consequences for group building and
  play styles (groups complement each other, enemies adapt to weaknesses).
- Legendary forms and classes can raise individual caps – also the cap of an imprint and
  the max level (see [Levelling up](#levelling-up-level-skills-imprint)).

### Damage

```
damage = base damage (weapon or spell form: material, rarity, weight)
       × (1 + sum of progress bonuses)           ← added up
       × situational factors (charge, chain, weak point)   ← multiplied
       → split by damage type (physical part, element part)
       → each part reduced by the target's resistance curve
```

- **Progress bonuses are added up** into one pool: skill level, skill titles, strength or
  dexterity, imprint (element part only), bonuses of the hilt. Multiplied, they would grow
  far faster than HP (about ×28 against ×3 at level 100).
- **Balancing target:** at level 100 with normal play the pool is about +200 %, so damage
  grows roughly like HP and the length of a fight between equals stays stable. Whoever
  plays hard has higher skills and lies above it.
- **No direct damage bonus per level** – the level acts through strength and dexterity.
- **Situational factors are multiplied:** charge, chain and weak point reward position and
  play in the moment; each is capped on its own and does not grow with the level. Dexterity
  raises only the weak-point factor.
- **Base damage comes from equipment** (material, rarity, weight) – the lever for reinvesting
  a bigger bar (see [items.md](items.md#what-makes-a-weapon-better)).
- **Strength or dexterity, with a smooth transition:** weapon weight ÷ the wielder's body
  weight gives a share "heavy" between 0 and 1. Bonus = share × strength bonus +
  (1 − share) × dexterity bonus. Both bonuses count only what level and training added on
  top of the species' base value – so a colossus' huge strength does not give absurd damage
  by itself; its large body weapons do that through their base damage. For a giant, a
  greatsword is a light weapon.
- Guard rail stays: against roughly equal opponents, no death from full health in one hit
  (see [combat.md](combat.md#high-damage-no-meat-shields)).

---

## Affinity and imprint

Two terms for two things – for monsters and humanoids alike:

| | What | Bound to | Changes |
|---|---|---|---|
| **Affinity** | how easy or hard an element is for someone | the species or race (innate) | only through evolution (see [Evolution](#evolution-monsters)) |
| **Imprint** | a developed alignment with an element, with strong bonuses | the character (acquired) | in levels, at milestones |

### Imprint and element skill are different things

- **Element skill** (fire magic, ice magic …) = **skill at casting** (active): spell forms,
  costs, range, field size. Skill titles (e.g. *Fire Mage*) belong only here.
- **Imprint** = **alignment of the being** (passive): resistance, an element touch on
  everything (weapons and augments, body skills, spells), visibility, opposites, affinity
  after evolution, requirement for evolutions.
- This way non-casters also get an element: the frost wolf bites with ice, the armored
  saurian needs no earth magic, a swordfighter with a fire imprint strengthens their fire
  augment. The physical imprint has no counterpart in magic anyway.

### Affinity

- Every species has **one affinity per element, physical included**. No second system: the
  affinity sets both the cap and growth of the matching resistance and how easily the
  matching imprint can be developed.
- Examples: saurians have a high earth affinity and accordingly a low lightning affinity
  (opposite element). A lightning saurian is possible but rare – and special for exactly
  that reason. Slimes have no preferred and no weak affinity; all elements are equally easy
  for them.

### Imprint

- **Progress through contact with the element** – not through skill XP: dealing element
  damage (whether by spell, augment or body skill), taking element damage, and exposing
  yourself to the environment. This way casting does not count twice. The affinity sets how
  fast.
- **Awakening:** the **first level** of an imprint awakens the element – it appears in the
  magic wheel, the element skill starts at level 1, and the **bolt** is available. After
  that the skill grows by use. This way every character gets magic, even monsters far from
  civilisation; exploring is rewarded (lava, heat, fire monsters …).
  - The first level is **much easier to reach** than the following ones – after that the
    element is easier to build up further with your own attacks.
  - For now no other ways (books, cores, teachers: rejected or postponed).
- **Focus pays off:** whoever focuses on one element collects all progress for it; whoever
  spreads out progresses more slowly on each. Several imprints are possible, but each
  additional one has a steeply rising threshold (as with augments).
- **Milestones:** for everyone at fixed level milestones (every 10 levels, matching the fact
  that skills are checked at level-up); for monsters also at every evolution. There an
  imprint can be newly developed or improved.
- **Five levels per imprint** as the normal limit; legendary forms and classes may raise it
  for their element (e.g. fire dragon: fire up to level 7). More milestones thus allow more
  breadth (more imprints, each weaker as multiple elements) or – legendary only – more
  depth. At each milestone you pick one option: start a new imprint (if the progress is
  enough) or improve an existing one by one level. Only what has enough collected progress
  is on offer – earned, but deliberately chosen.
- **Strong bonuses** for the element: resistance, damage, element touch of body skills and
  augments.
- **Opposites exclude each other:** no fire and ice imprint at the same time – matching the
  rule that immunity weakens the opposite element. Only special forms (e.g. prism slime,
  primordial slime) may break this.
- **Physical imprint:** a path of its own for pure body fighters.
- **On evolution** imprints are reset; they turn into a raised affinity of the new form
  (see [Evolution](#evolution-monsters)).
- **Visible:** an imprint can be seen on the character (material, colour, effects) –
  important for the guiding principle, because the opponent must be able to counter with
  the opposite element.

### Body form and imprint as two axes (monsters)

- **Body form** (evolution branch, e.g. armored saurian): model, animations, base behaviour
  – the expensive part.
- **Imprint** (optional): designed once per element and can be laid onto any form – shifts
  resistances, gives body skills an element touch (charge with a frost trail), changes the
  look through material and effects. No new model, no new animations.
- So an ice armored saurian is armored saurian plus ice imprint. New forms or elements cost
  work only once instead of once per combination.
- **Hand-made highlights:** some combinations still get their own forms that can do more
  than the sum (e.g. the horned saurian: saurian body plus fire imprint).

### Humanoids

- The same mechanics as for monsters (affinity of the race, imprint of the character); the
  imprint shows in the look. There are no titles from imprints – *Ice Mage* is a skill
  title of ice magic.

---

## Two kinds of XP: nXP and hXP

- Every defeated enemy gives two kinds of XP:
  - **nXP (normal XP)** for the character level. Always the same per monster type and
    level, independent of your own level.
  - **hXP (hardship XP)**, hidden. They come from **comparing the threat ratings** of the
    enemy and your own side (see below) – the more dangerous the enemy is in relation, the
    more hXP.
- Example: a normal slime and an elemental slime, both level 10, give the same nXP. But the
  elemental slime is more dangerous and gives more hXP – and even more if you yourself are
  clearly less dangerous.
- **Threat rating instead of level comparison:** a formula calculates a value for the
  danger of a character from several factors (species or race, base values, skills,
  imprints …). Comparing the threat ratings gives the hXP yield.
  - The formula is easy to readjust in tests and during live operation.
  - It can take **groups** into account directly (threat of the group against that of the
    enemy).
  - It solves the problem that monsters start again at level 0 after an evolution: the
    actual danger is compared, not the level.
- The **ratio of hXP to nXP** of a fight sets how much the skills grow from that fight. So
  hard fights always give relatively more skill growth.
- Aggravating conditions raise hXP only if they **actually take effect**: whoever walks
  through the cold in warm armor is not exposed to the cold and gets no bonus.
- Over the character's lifetime the hXP give a **hardship rating** that counts for rare
  evolutions (see below). It is the **ratio of hXP to nXP**, not the sum: this way many easy
  fights do not give the same as a few hard ones, and the value scales with the character.

---

## Levelling up: level, skills, imprint

### Character level

- **Pacemaker and base values:** the level sets *when* skills can level up and when
  milestones come (imprint, evolution, hidden classes and titles). On top of that each
  level raises the body attributes by 2 % of their base value – so more HP, a bigger bar and
  more damage through strength and dexterity. There is **no direct HP or damage bonus per
  level**. Most of the strength still comes from skills, attributes and equipment.
- **Max level (guideline values, valid until further notice):** because skills and imprints
  are reset on evolution, an upper limit for the total level is no longer needed; each form
  has its own cap.

  | Who | Max level |
  |---|---|
  | Humanoids (all races) | 100 |
  | Humanoids with a hidden or epic class or title | 120 |
  | Monsters, normal form | at most 60 (starting forms that lead directly to rare forms may be higher) |
  | Monsters, **rare** form | 100 |
  | Monsters, epic form | 120 |
  | Monsters, legendary form | 150 |

  - **Rare forms** are a manually assigned label for the form that the great majority reach
    as the final form of their character – usually the last form before epic or legendary.
    Hence cap 100, as for humanoids.
  - Humanoid races differ through affinities, base values, and caps and growth of
    attributes – not through more levels.
  - Legendary humanoids are not planned for now.
- Most players reach roughly their training cap of the attributes at max level.

### Evolution (monsters)

- **No fixed tiers:** each form has a **rarity** (normal, rare, epic, legendary), its **own
  level cap** and possible **predecessors**. The tree decides where you can evolve to – in
  the extreme case a legendary form can already be the second form.
- **Evolution at every milestone** (every 10 levels), if the requirements are met (play
  style: skills, attributes, imprint; for rare forms e.g. the hardship rating). At the
  latest at the level cap of the current form.
- **Rare forms are limited by their requirements** (imprints, values, hardship rating …),
  no longer by a level budget. Epic and legendary forms therefore need sufficiently hard
  requirements.
- **Evolve early or late:** early = faster to stronger forms; late = stronger instincts and
  affinities that make the next form easier. Whoever evolves early fights longer with weak
  skills against stronger enemies – which gives many hXP anyway because of the threat
  rating ("playing hard pays off").
- The requirements should be hinted at in the game (lore, inhabitants), so that players
  decide deliberately.
- **No going back:** after an epic form you cannot go back to a normal one.
- **Back to level 0 after evolution:** base values reset to the level, but are higher
  thanks to the new form.
- **Skills and imprints are reset** – you first have to get to know the new body. What you
  built up before is kept as **instinct** or **affinity**:
  - **Instinct (for skills):** for every skill you had developed before the evolution you
    get, in the new form, a **small permanent bonus** on exactly that skill **and relearn
    it faster** (more skill XP). Its strength depends on how high the skill was relative to
    the cap of the old form. Instincts add up over several evolutions, with diminishing
    gains. If the new form also has the skill (e.g. the same body skill), the instinct takes
    effect directly; otherwise it is kept and takes effect as soon as a later form has the
    skill again (proposal).
  - **Affinity (for imprints):** every imprint becomes a raised affinity for that element in
    the new form – it is easier to develop again, and the resistance benefits a little.
  - **Special trait of some forms:** the elemental slime (and similar forms to come) takes
    one imprint at level 1 along as an extra evolution bonus.
- This gives **evolve early or late** even more weight: whoever stays in a form for a long
  time takes stronger instincts and affinities along; whoever evolves early (for example
  for a rare form) gives them up. Legendary forms draw their strength from the form, not
  from skills carried over.
- Not every branch needs an epic or legendary form.
- **This applies to Phase 2.** For the **MMO (Phase 3)** epic and legendary forms are to be
  unlocked differently: similar to the second-generation legendary weapons in Guild Wars 2 –
  a combination of achievements and collecting large amounts of resources. Fine-tuning only
  with experience from Phase 2.
- **Starting forms always open the physical form at the level cap** – so every monster
  evolves at least once. Beyond that, mainly the elements (imprints) decide which
  direction it goes next; there is almost always a physical direction.
- At the evolution step you choose body form and imprint together.

### Skills

- **Every skill is capped at a fixed 150.**
- **At most one level per level-up and skill.** Whoever plays hard levels up their skills at
  almost every level-up; whoever takes it easy falls behind – "same level, very different
  strength".
- **Up to max level** skills are never above the level of the current form; **after that**
  they grow through **veteran levels** up to 150 (see below).
- **Surplus skill XP** does not expire; it waits for the next level-up.
- **No decay** of unused skills (frustrating, leads to mandatory grinding).
- **No skill upgrades:** skills simply keep levelling as long as possible.
- **Skill titles every 30 levels** (30, 60, 90, 120, 150 – five per skill): they raise the
  **damage** of the matching weapon or element by a percentage (added to the pool of
  progress bonuses); for skills without damage (shield fighting, crafting …) the **core
  effect** of the skill (e.g. block arc, efficiency). Titles may also passively raise
  **maximum HP or the maximum reserve**. Titles can also be **requirements for
  evolutions** – especially for elemental magic and **in addition** to the imprint, where
  casting is concerned (e.g. prism slime: imprints and a magic title). The imprint stays the
  main requirement because it describes the being.
- **Traits between the titles** (as in Guild Wars 2): at levels 10, 20, 40, 50, 70, 80, 100,
  110, 130 and 140 a **choice of three traits** opens up – two choices per section, ten per
  skill. Rhythm: choice, choice, title. Traits give **properties**, titles give the
  percentages. Freely changeable outside combat; for monsters reset together with the
  skills on evolution. Example traits:
  - Sword fighting – the chain no longer ends from a single enemy hit.
  - Traits give no numbers: they raise no attributes. (A short-range teleport instead of the
    dodge step is no longer a trait but an alternative dodge capability.)
- Skill titles and traits need no hardship rating – only the skill level. Hidden classes and
  titles such as *Dragonslayer* still depend on special achievements or on the hardship
  rating.
- Properties of traits and of items (e.g. masterworks) must not overlap.

### Veteran levels (after max level)

- After max level you keep collecting **nXP** and gain **veteran levels**. The XP needed
  keeps rising linearly.
- **Each veteran level raises one skill by one level** – freely chosen, up to the limit of
  150. Much slower than before max level (where several skills can rise per level) and
  harder with every level – a soft limit instead of a hard cap.
- Because it runs on nXP, everyone progresses evenly (see Fairness for all play styles in
  [overview.md](overview.md)).

### Milestones

- Every 10 levels: choose an imprint (see [Affinity and imprint](#affinity-and-imprint)) –
  for humanoids ten milestones, so e.g. two imprints at level 5. For humanoids also a check
  for hidden classes and titles (from special achievements).
- Monsters: also at every evolution.

### Respawn penalty

- **Whoever dies loses a share of their collected skill XP** that is still waiting for the
  next level-up. A skill level-up at the next level-up becomes much less likely; skill
  levels already reached stay.
- **Share = 2 × the relative remainder of nXP needed for the next level**, at most 100 %
  (skill XP drops at most to 0). The share refers to the skill XP **already collected**, not
  to the amount needed for a level. Examples: if 30 % of the nXP are still missing, 60 % of
  the collected skill XP are lost; if 5 % are missing, only 10 %; from 50 % missing nXP
  upwards, everything is lost.
- **This also applies to carried-over skill XP**, for example when at the last level-up you
  could theoretically have gained two skill levels. You can thus lose more than a whole
  skill level's worth of collected skill XP.
- Consequence: the first half of a level is the dangerous phase; towards the end of the
  level a death becomes cheaper and cheaper.
- Reaching max level **without dying** is rewarded this way: these characters have the
  strongest skills.
- Lying on the ground and being helped up does not count as death; giving up yourself does.
- This also covers the friendly-fire rules: someone always pays the respawn penalty.

### Arena (stage 1)

- Start at fixed levels (e.g. just before a level cap), so that evolutions can be tested
  directly; no permanent progress and no respawn penalty. Slow growth begins from stage 2.

---

## Group fights

- Everyone involved gets the full nXP – **with no minimum participation**.
- **hXP simply split:** within a group, the hXP are divided evenly by the number of group
  members. Deliberately simple instead of a formula based on participation.
- **Several groups** in one fight: the hXP are first distributed to the groups by the
  **damage of each group**, then evenly within each group. Whoever fights alone counts as a
  group of their own.
- **The threat rating is calculated for the whole group**, because a fight as a group is
  much easier than alone. So an enemy gives a strong group few hXP in total – this also
  limits power-levelling: whoever lets a much stronger group carry them gets full nXP but
  hardly any hXP, levels along and ends up with a weak character. Groups have to look for
  stronger enemies to grow well together.
- **Overview after the fight (contribution):** who contributed how much damage, healing,
  blocked hits and so on – for information only (does not decide hXP). An incentive for
  support roles; in the arena helpful for tests and to make rank changes understandable.
- Rejected: splitting hXP by effect points – too complex.
