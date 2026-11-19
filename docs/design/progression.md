# Progression

This document holds the full progression design: character level, skills, attributes,
affinity and imprint, the three kinds of XP (nXP, hXP, rXP), levelling up, evolution, skill
titles and traits, renown and the Adventurers' Guild, milestones, the respawn penalty, and how
XP is shared in group fights.
Terms follow `GLOSSARY.md`. Numbers are guideline values unless stated otherwise; all
gameplay numbers live in `data/values.json`.

---

## Level, skills and attributes

Three separate pillars that grow in different ways:

| | What | Grows through | Level-dependent | hXP |
|---|---|---|---|---|
| **Character level** | base level | nXP from defeated enemies | – | – |
| **Skills** (active) | what you *do* – attack and defence alike (sword fighting, fire magic, shield fighting, weapon smithing) | automatic skill points at a level-up (distributed by hXP) plus free skill points from renown | the automatic part up to your level | yes |
| **Renown level** | experience as an adventurer | rXP from deeds (quests, events, discoveries …) | no | no |
| **Attributes** | what you *are* – resistances and body attributes | body attributes: level (+2 % of the base per level) plus strain; resistances: exposure only | body attributes: partly | no |

### Skills

- A skill level is the sum of two kinds of **skill points** (details in
  [Skill points](#skill-points-automatic-and-free)):
  - **automatic skill points** at every level-up – their number depends on how hard you
    fought, and they go to the skills you used, in proportion to the hXP each one got;
  - **free skill points** from the renown level – you assign them yourself.
- hXP are credited only when an enemy is defeated. So letting friends beat you up only pays
  off if the other person is defeated at the end and accepts a respawn penalty.
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
| **Crafting** (6) – practice skills | `weapon_smithing`, `armor_smithing`, `weapon_carving`, `armor_carving`, `leatherworking`, `tailoring` (see [items.md](items.md#crafting-skills)) |
| **Leadership** (1) – practice skill | `leadership` (see [Leadership](#leadership)) |
| **Body skills** (monsters only) | active ones from the shared catalogue: slam, projectile, bite, charge, pounce, howl/roar, tail strike, stomp, threads/web (see [monsters.md](monsters.md#body-skills-monsters)) |

- A humanoid has 23 skills; monsters add their body skills and use no weapon skills.
- **Two kinds of skills:**

  | | Combat skills | Practice skills |
  |---|---|---|
  | Which | weapon families, elements, body augment, body skills | the six crafting skills, leadership |
  | Growth | automatic skill points at a level-up, distributed by hXP | **their own XP by practising**, once a condition is met – independent of level, nXP, hXP and the skill-point budget |
  | Limit | automatic part ≤ own level; 150 | 150; the XP needed rises per level (soft limit) |
  | Traits and titles | yes | yes |
  | Free skill points (rXP) | yes | yes |

  Practice skills give no power on their own in a fight, so they need no budget; they never
  compete with the combat skills at a level-up.
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
  strike trains strength, separately from the hXP of the skill). Growth depends on intensity –
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
- **Base damage comes from equipment** (schematic, material, rarity) – the lever for
  reinvesting a bigger bar (see [items.md](items.md#what-makes-a-weapon-better)).
- **Strength or dexterity – fixed per schematic:** each weapon schematic has a fixed share,
  e.g. dagger 0/100, longsword 50/50, greatsword 100/0 (table in
  [combat.md](combat.md#schematics-weight-and-stat)); short bow dexterity, longbow strength;
  each **body skill** of a monster is tied to one of the two (e.g. slam, charge, stomp →
  strength; bite, pounce, tail strike → dexterity). Bonus = strength share × strength bonus +
  dexterity share × dexterity bonus. Both bonuses count only what level and training added
  on top of the species' base value – so a colossus' huge strength does not give absurd
  damage by itself. The grip (one- or two-handed) does not change the share or the damage.
- **Guide values (guess):** 0.3 damage bonus per gain for strength and for dexterity (gain =
  value ÷ base − 1); at level 100 without training that is +60 %, about a third of the
  targeted pool.
- Guard rail stays: against roughly equal opponents, no death from full health in one hit
  without a clear telegraph – held by balancing and armor, not by a cap (see
  [combat.md](combat.md#high-damage-no-meat-shields)). Base damage per schematic follows
  from its stamina cost (see [combat.md](combat.md#base-damage-from-the-cost)).

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

- **Progress through contact with the element** – not through hXP: dealing element
  damage (whether by spell, augment or body skill), taking element damage, and exposing
  yourself to the environment. This way casting does not count twice. The affinity sets how
  fast.
- **Awakening:** the **first level** of an imprint awakens the element – it appears in the
  magic wheel, the element skill starts at level 1, and the **bolt** is available. After
  that the skill grows by use. This way every character gets magic, even monsters far from
  civilisation; exploring is rewarded (lava, heat, fire monsters …).
  - The first level is **much easier to reach** than the following ones – after that the
    element is easier to build up further with your own attacks.
  - **Second way: unlocking.** A free skill point (at a teacher) unlocks the element
    skill at level 1 – but **not** the imprint (see
    [Unlocking skills](#unlocking-skills-teachers-and-learning-by-use)). Books and cores stay
    rejected.
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

## Three kinds of XP: nXP, hXP and rXP

| | Source | What for |
|---|---|---|
| **nXP** (normal XP) | defeated enemies – always the same per monster type and level, independent of your own level | the **character level** → attributes (+2 % of the base per level), milestones, automatic skill points |
| **hXP** (hardship XP), hidden | defeated enemies – **always at least as many as nXP**, more the more dangerous the enemy is in relation (threat ratings) | the **hardship rating** (hXP ÷ nXP) and the **key** that distributes the automatic skill points |
| **rXP** (renown XP) | **deeds**: quests, dynamic events in the open world, discoveries, dungeon floors, champion bosses and more; at max level nXP flow into rXP | the **renown level** → one **free skill point** each; the adventurer rank (see [Renown](#renown-and-the-adventurers-guild)) |

### hXP

- Example: a normal slime and an elemental slime, both level 10, give the same nXP. But the
  elemental slime is more dangerous and gives more hXP – and even more if you yourself are
  clearly less dangerous. Against an equal or weaker enemy hXP = nXP.
- **Threat rating instead of level comparison:** a formula calculates a value for the
  danger of a character from several factors (species or race, base values, skills,
  imprints …). Comparing the threat ratings gives the hXP yield.
  - The formula is easy to readjust in tests and during live operation.
  - It takes **groups** into account directly (threat of the group against that of the
    enemy, see [Group fights](#group-fights)).
  - It solves the problem that monsters start again at level 0 after an evolution: the
    actual danger is compared, not the level.
- **Credited only on a win:** hXP are credited when an enemy is defeated; being defeated
  loses only what is pending since the last enemy defeat.
- **Distributed by use:** at the end of a fight the hXP are assigned to the skills whose
  actions you used (sword strike, fire bolt …), **in proportion to the stamina and mana each
  skill spent**. Mana bound by an augment counts as amount × time. Movement (sprinting,
  dodging) belongs to no skill and only trains attributes. Practice skills (crafting,
  leadership) grow by their own XP, outside this distribution.
- This cannot be gamed: spending stamina only shifts *which* skill grows, not *how much* –
  the amount depends on the enemy alone.
- **Harsh conditions give no extra hXP** (cold, darkness, low reserve, no potions): they make
  the fight harder for both sides. Hardship comes only from the threat rating – nothing to
  trick.
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
- **Skills and imprints are reset** – you first have to get to know the new body. The
  **renown level stays**, and the free skill points are refunded once for free (see
  [Skill points](#skill-points-automatic-and-free)). What you
  built up before is kept as **instinct** or **affinity**:
  - **Instinct (for skills):** for every skill you had developed before the evolution you
    get, in the new form, a **small permanent bonus** on exactly that skill **and relearn
    it faster** (a larger share of the automatic points). Its strength depends on how high the skill was relative to
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

- **Every skill is capped at a fixed 150** (automatic plus free points).
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

### Skill points: automatic and free

**Automatic skill points** come with every nXP level-up:

- **Number = 1 + 2 × log₂(hardship rating)** since the last level-up (hardship rating =
  hXP ÷ nXP, at least 1). Every doubling of hardship gives two more points; no hard cap,
  the gain just flattens. Factor and base are tuning screws.

  | Hardship rating | 100 % | 150 % | 200 % | 300 % | 400 % | 800 % | 1,600 % |
  |---|---|---|---|---|---|---|---|
  | Points | 1.0 | 2.2 | 3.0 | 4.2 | 5.0 | 7.0 | 9.0 |

- **Distribution** in proportion to the hXP each skill got since the last level-up,
  **with fractions:** every skill has a progress bar (0–1); the whole part becomes a skill
  level, the rest stays. No hXP are lost and there are no thresholds to miss narrowly.
- **Limit:** the automatic part of a skill is at most your own level (for monsters the level
  of the form). A share above it goes to the other used skills, or waits for the next
  level-up if there are none.
- **A budget, not breadth:** with normal play a character has about 100 automatic points at
  level 100, playing very hard up to about 300. Eight elements at 100 would need 800 – an
  all-rounder is not possible; you build your character deliberately. "Same level, very
  different strength" stays: whoever plays hard has more points.

**Free skill points** come from the renown level (one per renown level):

- Assigned to **any skill available** to the character – also above your own level – or used
  to **unlock** a new skill (see [Unlocking skills](#unlocking-skills-teachers-and-learning-by-use)).
- They can be **reassigned for a cost** (set with the economy). Automatic points cannot.
- **At 150 an automatic point pushes out a free one**, and that free point becomes available
  again at no cost.
- **On evolution (monsters)** the renown level stays; the free points are refunded **once for
  free** and can only go to skills the new form has. This softens the weak phase after an
  evolution on purpose – long play is rewarded.
- Monsters deliberately have less access to rXP at first (few quests outside cities) and get
  it fully only late, with the humanoid form.
- **Fairness:** the automatic points follow the hXP ("playing hard pays off"); the free points
  are how casual players catch up – from the start instead of only after max level.

### Unlocking skills: teachers and learning by use

- **A free skill point can unlock any skill the character could learn** (it becomes level 1;
  one point is enough):
  - weapon and crafting skills: always, for humanoids;
  - element skills: for everyone, **without the imprint**;
  - body skills: only those the form lists as **accessible**;
  - capabilities: never – they belong to the body.
- **Two ways to an element:**

  | | Awakening (contact) | Unlocking (free point) |
  |---|---|---|
  | Gives | element skill level 1 **and** imprint level 1 (resistance, damage, access to evolutions) | **only** the element skill level 1 – you can cast |
  | Price | exploring, time, risk | one free point from the budget |

  Whoever buys fire magic casts fire but is **no fire being**: no imprint bonus, no access to
  fire evolutions. The imprint stays the way for "the being", unlocking the way for "the
  ability". Opposing imprints stay excluded; opposing element skills are possible (the magic
  wheel does not select both at once).
- **Teachers** do the unlocking: for humanoids in cities and at the Adventurers' Guild; also
  with some **travelling merchants** – so monsters get access too, harder and by luck (for
  them mainly element skills). Monsters unlock their accessible body skills "by instinct",
  without a teacher.
- **Learning by use:** a skill at level 0 can also be unlocked by using it. Whoever fights
  with a sword without sword fighting gets hXP on it and unlocks it with an automatic skill
  point at the next level-up. During this **learning phase** a weapon family at level 0 has
  **+100 % demand** (shield: holding the block; bow: drawing); from level 1 on the malus is
  gone. A switch, not a curve – a deliberate exception to "skills do not lower costs" that
  cannot multiply. Because hXP follow the stamina spent, the doubled cost even makes a new
  skill catch up faster at first. Spells need the element skill at all (the spell forms hang
  on it), so magic has no malus.
- **Tutorial (for later):** every character gets at least one free skill point in the
  tutorial and spends it there at a teacher, e.g. on a weapon from the rack of wooden starter
  weapons or on an element (see [monsters.md](monsters.md#tutorial)). This replaces the
  earlier starting alignment.

### Renown and the Adventurers' Guild

- **Renown level:** rises through rXP; the XP needed rises linearly – a soft limit. It replaces
  the earlier veteran levels, which only started after max level.
- **Adventurers' Guild** (a proper name; "guild" alone means a player guild): a neutral NPC
  institution with guild halls in the cities. Humanoids and **Shifters** can register;
  untransformed monsters cannot.
- **Adventurer rank** follows from the renown level, in metal tiers: **copper, iron, bronze,
  silver, gold, mithril, adamant** (thresholds in balancing, e.g. one rank every 10 renown
  levels). ("Rank" alone stays the ranked matchmaking value.)
- rXP are collected without the guild too; on registering, the guild ranks you by the renown
  you already have.
- **What the rank gives – no stats:**
  - **jobs by rank** on the guild's job board (harder and more rewarding quests, so more rXP);
  - a **better starting reputation** in cities that trust the guild – valuable above all for
    Shifters (see cities and relations in [world.md](world.md));
  - a **visible badge**. It shows experience, not strength.
- Later: **rank exams** from a middle rank on (a special quest or fight).

### Milestones

- Every 10 levels: choose an imprint (see [Affinity and imprint](#affinity-and-imprint)) –
  for humanoids ten milestones, so e.g. two imprints at level 5. For humanoids also a check
  for hidden classes and titles (from special achievements).
- Monsters: also at every evolution.

### Respawn penalty

- **Whoever dies loses a share of the hXP collected since the last level-up.** That lowers
  the hardship rating of this level and so the number of automatic skill points at the next
  level-up (at least one point always remains, because the rating never drops below 1).
  Skill levels and progress bars already reached stay.
- **Share = 2 × the relative remainder of nXP needed for the next level**, at most 100 %.
  The share refers to the hXP **already collected** in this level. Examples: if 30 % of the
  nXP are still missing, 60 % of the collected hXP are lost; if 5 % are missing, only 10 %;
  from 50 % missing nXP upwards, everything is lost.
- Consequence: the first half of a level is the dangerous phase; towards the end of the
  level a death becomes cheaper and cheaper.
- Reaching max level **without dying** is rewarded this way: these characters have the most
  automatic skill points.
- Lying on the ground and being helped up does not count as death; giving up yourself does.
- This also covers the friendly-fire rules: someone always pays the respawn penalty.

### Arena (stage 1)

- Start at fixed levels (e.g. just before a level cap), so that evolutions can be tested
  directly; no permanent progress and no respawn penalty. Slow growth begins from stage 2.

---

### Leadership

- **Leadership** (`leadership`) is a practice skill: whoever **leads a group** gets
  leadership XP whenever the group **wins a fight** – the more members took part (within the
  radius, in the won fight), the more. Inactive members do not count. Leading costs nothing
  and takes nothing from the combat skills; free skill points can speed it up.
- **Group size:** everyone can lead a group of **5**. Leadership raises the limit (e.g. 10,
  15, 20 for war bands – numbers in balancing). The base size is part of the basic promise:
  friends are never locked out.
- **Leader auras:** higher levels unlock auras like the howl (they bind stamina, work within
  a radius, do not stack per kind – see [monsters.md](monsters.md#auras-howl-and-roar)), but
  they are **only active while the character leads the group**; handing over the lead
  switches them off. Examples: allies regenerate stamina faster, the group's detection radius
  shrinks while sneaking, members resist interrupts better. The **banner** of the strategy
  layer is a leader aura of a high level.
- The **threat rating of the group** counts its leader auras, so a well-led group gets a
  little less hXP per enemy.
- Monsters learn leadership the same way (a wolf leading a pack). For the **alpha wolf**,
  leadership could be one of the evolution requirements.
- **Two levels, both kept:** leadership is **practice** – how well you lead a group, in
  everyday PvE and PvP. The **following** of the strategy layer is **trust** – how many
  players actively follow you; it unlocks orders to NPC units, fiefs and the title "leader"
  (see [strategy.md](strategy.md#leaders)). A ruler needs both.

## Group fights

- Everyone involved gets the full nXP – **with no minimum participation**.
- **The threat rating is calculated for the whole group**, because a fight as a group is
  much easier than alone. Every member then gets the **full hXP** that come from comparing
  the group's threat rating with the enemy's – there is no splitting within the group. Each
  member distributes them to their own skills by their own use.
- **Several groups** at the same enemy (e.g. a world boss) count as **one large group** with
  a common threat rating.
- So an enemy gives a strong group few hXP – this also
  limits power-levelling: whoever lets a much stronger group carry them gets full nXP but
  hardly any hXP beyond nXP, levels along and ends up with few skill points. Groups have to look for
  stronger enemies to grow well together.
- **Overview after the fight (contribution):** who contributed how much damage, healing,
  blocked hits and so on – for information only (does not decide hXP). An incentive for
  support roles; in the arena helpful for tests and to make rank changes understandable.
- Rejected: splitting hXP by effect points – too complex.
