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
| **Skills** (active) | what you *do* – attack and defence alike (swordsmanship, fire magic, dodge, block) | using them in combat | yes | yes |
| **Attributes** | what you *are* – resistances and body attributes | exposure or strain | no | no |

### Skills

- Skills level up by use, not through skill points. Each skill collects its own skill XP.
  Skill XP is credited at the end of a fight – only on a win. At every character level-up
  the game checks which skills have enough skill XP to level up as well (details in
  [Levelling up](#levelling-up-level-skills-imprint)).
- So letting friends beat you up only pays off if the other person is defeated at the end
  and accepts a respawn penalty.
- **Skills must have a noticeable effect.** Whoever dodges a lot dodges noticeably better
  (a longer and faster dodge step, less stamina) and so reaches flank and back more easily.
  A skill level-up should be felt in the hand, not only in a number.

### Attributes: resistances and body attributes

- **Resistances** (physical, magical, fire, ice …) grow only through exposure.
  - Hits in combat count as external influence and give resistance XP directly on every
    hit – much faster than through the environment. This applies to hits from monsters
    (NPCs); for hits from players see Friendly fire and reputation in [world.md](world.md).
  - The environment also works (staying near fire for a long time, ice bathing), but only
    with a massive time investment. Whoever deliberately spends many hours on it may be
    rewarded – but has to take care not to die doing it.
- **Body attributes** work the same way, but through strain close to your own limit instead
  of through exposure. This also counts directly per action in combat – a sword strike
  trains strength, separately from the skill XP for swordsmanship. Proposal:

  | Attribute | Grows through | Affects |
  |---|---|---|
  | **Strength** | swinging heavy weapons, carrying heavy loads, mining ore, chopping wood | damage of heavy weapons, carrying capacity |
  | **Endurance** | running, swimming, climbing, flying, fighting until stamina is really empty | stamina: maximum and regeneration |
  | **Dexterity** | climbing, balancing, ranged hits, precise crafting work | attack speed, precision, fall damage |
  | **Vitality** | surviving heavy damage, healing injuries, holding out long on low reserve | maximum HP, healing rate, reserve |
  | **Spirit** | using mana until exhausted | mana: maximum and regeneration |
  | **Perception** | recognising telegraphed attacks and dodging them, discovering hidden enemies, exploring | detecting hidden enemies; at higher levels, recognising telegraphed area attacks earlier and better |

  Endurance, spirit and vitality form a triangle with the three bars stamina, mana and
  reserve (see Stamina, mana and reserve in [magic.md](magic.md)).
- **Independent of level and capped.** Most players reach roughly their cap when they reach
  max level. Whoever grinds early (e.g. lots of ice bathing at level 1) has a head start
  later in the game – but no permanent advantage.
- **Growth depends on intensity:** only exposure that really hits you relative to your own
  value counts. As cold resistance grows, a cold lake gives less and less; then you need
  glaciers in the high mountains. So early grinding is limited by the world instead of by
  the level, and whoever wants more has to explore. Conditions count only if they actually
  take effect (warm armor → no cold).
- **hXP do not flow into attributes.** The hardship system rewards skills and rare
  evolutions.

### Species/race and skills shape the attributes

- **Base value, cap and growth** depend on the race or species – through its
  **affinities** (see [Affinity and imprint](#affinity-and-imprint)). A fire salamander
  starts with high fire resistance and can push it far; monster evolutions can shift these
  values.
- **Skills raise or lower attributes by a percentage.** Fire magic raises fire resistance
  but lowers ice resistance; ice magic the other way round.
- **Opposing elements** create trade-offs. Three pairs and three loners (poison, wind,
  physical).

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
- **Incorporeal beings** (e.g. ghosts) are naturally immune to physical damage, but weak
  against all magical damage types.
- **Immunity is possible, but it costs:** whoever becomes immune to a damage type has a
  much lower resistance to the opposite element. This allows highly specialised characters
  – with consequences for group building and play styles (groups complement each other,
  enemies adapt to weaknesses).
- Legendary forms and classes can raise individual caps – also the cap of an imprint and
  the max level (see [Levelling up](#levelling-up-level-skills-imprint)).

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
  milestones come (imprint, evolution, hidden classes and titles). On top of that a little
  more HP and base damage per level – but most of the strength comes from skills,
  attributes and equipment.
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
- Most attributes reach roughly their cap at max level.

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
  **damage** of the matching weapon or element by a percentage; for skills without damage
  (dodge, block, stealth, crafting …) the **core effect** of the skill (e.g. dodge
  distance, damage reduction, efficiency). Titles can also be **requirements for
  evolutions** – especially for elemental magic and **in addition** to the imprint, where
  casting is concerned (e.g. prism slime: imprints and a magic title). The imprint stays the
  main requirement because it describes the being.
- **Traits between the titles** (as in Guild Wars 2): at levels 10, 20, 40, 50, 70, 80, 100,
  110, 130 and 140 a **choice of three traits** opens up – two choices per section, ten per
  skill. Rhythm: choice, choice, title. Traits give **properties**, titles give the
  percentages. Freely changeable outside combat; for monsters reset together with the
  skills on evolution. Example traits:
  - Swordsmanship – the chain no longer ends from a single enemy hit.
  - Dodge (high level) – the dodge step becomes a short-range teleport, for example behind
    the enemy.
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
