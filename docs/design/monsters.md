# Monsters, species and characters

This document holds the full design for monster lines and their forms, body skills,
legendary forms, the humanoid counterpart, the humanoid form for monsters, and the choices
at character creation (flow, look, size, unlocking species). Terms follow
`GLOSSARY.md`. Evolution, level caps and imprints in general are described in
[progression.md](progression.md); magic in [magic.md](magic.md); cities and relations in
[world.md](world.md).

---

## Body skills (monsters)

Special monster skills are tied to the **body**, not to the species. There is one **shared
catalogue**, and which skills a form can learn depends on its body features. A skill is
designed once and applies to every form with a matching body.

| Body feature | Body skill (examples) | Forms that could have it |
|---|---|---|
| Ranged attack from the body | **projectile** | slimes, saurians with plates, spiders |
| Horns, heavy head | **charge** (ramming attack out of a sprint) | saurians, later other horned creatures |
| Jumping legs | **pounce** (leap out of a run) | wolves, cats |
| Endurance runners | **pursuit** (while chasing, movement costs less stamina) – a capability | wolves |
| Jaws, bite | **bite** (the basic attack, like the slime's slam); **wear down** (a bite costs the target stamina) is a property or trait of bite | wolves, other pursuit hunters |
| Voice | **howl** or **roar** – an **aura** that strengthens allies nearby (see [Auras](#auras-howl-and-roar)) | wolves, maybe dragons |
| Tail | **tail strike** (variants: club to the rear, stinger, whip with long reach) | saurians, lizards |
| Heavy body | **stomp** (area attack) | colossi |
| Neck frill, armor plate at the front | **front shield** (blocks only to the front, very strong there) – a capability | horned saurians |
| Wings | **glide**, **fly** – capabilities | flying lizards, dragons |
| Spinnerets | **threads**, **web**; **wall-walking** – a capability | spiders |

- **Active and levelling:** slam, projectile, bite, charge, pounce, tail strike, stomp,
  threads/web. **Howl/roar** is a levelling **aura** (passive while switched on, see
  [Auras](#auras-howl-and-roar)). **Passive ones are capabilities** (pursuit, front shield, glide/fly,
  wall-walking): they do not level and sit in the form's list (see
  [Capabilities](#capabilities)).
- **Every body skill that uses stamina needs two values** (set per skill, or per variant, in
  the data): a **strength / dexterity share** for its damage bonus – with steps in between,
  as for weapon schematics – and a **cost ratio**, the counterpart of weapon weight ÷ body
  weight. Body skills that use **mana** (projectile, threads/web) need neither; they work like
  spells.

  | Body skill | Strength / dexterity | Cost ratio (guess) | Why |
  |---|---|---|---|
  | stomp | 100 / 0 | ~0.06 (like a greatsword), area attack | pure weight |
  | charge | 80 / 20 | **whole body** (like a dodge: total weight ÷ body weight × base cost) | mass and momentum, a little aiming |
  | tail strike: club (saurian) | 80 / 20 | ~0.03 | heavy blow to the rear |
  | slam (slime) | 70 / 30 | ~0.01 | the body as mass, but a gel body also snaps. (0.038 = 0.3 ÷ 8 came from the old cost formula ÷ strength; with the cost from ÷ body weight a slam would cost more than a slime's whole bar. ~0.01 gives about two slams in a row at level 0, like the longsword for a human.) |
  | bite (wolf) | 40 / 60 | ~0.02 (like a short sword) | jaw force, above all a fast snap |
  | claws (if they become a skill) | 40 / 60 | ~0.02 | like a short sword |
  | pounce | 30 / 70 | **whole body**, like charge | leaping power, above all body control |
  | tail strike: whip (long reach) | 30 / 70 | ~0.03 | speed at the tip, little mass |
  | tail strike: stinger | 10 / 90 | ~0.02 | an aimed stab, like a dagger |
  | howl / roar | – | binds stamina (aura) | no damage |
  | projectile, threads/web | – | mana | like a spell |

  This gives the lines a direction by itself: saurians reward strength, wolves rather
  dexterity, the slime lies in between.
- **How a form gets its body skills:** some **base skills come with the form** – it feels
  different at once. Other skills are only **accessible**. They are learned by use or under
  a condition (e.g. a minimum value in an attribute). So there is still something to
  discover.
- Body skills **grow by use** like all skills and are reset on an evolution. The instinct
  takes effect at once if the new form has the skill too.
- Many body skills change or replace a general skill (e.g. a thread instead of a roll when
  dodging). This fits the branches agile, armored and colossus.
- **Lines differ** through the combination of their body skills, their base values,
  affinities and properties of the form – not through exclusive skills.
- **Unique remain the rule-breaks of legendary forms** (e.g. Fenrir, primordial slime).
  They are tied to the single form.
- **Humanoids** have no access to the catalogue. Their counterpart is weapons, magic and
  hidden classes (see [Counterpart for humanoids](#counterpart-for-humanoids)).
- **Flying without hard limits:** flying is in theory unlimited. It is balanced through the
  rest of the balancing. Flying forms have lower stats, and dodging in the air is harder –
  they are an easier target, especially for ranged combat.
- **Flying uses stamina** (like sprinting, swimming, climbing). So how long you stay in the
  air depends on endurance and skill rather than a fixed rule. Whoever attacks while flying
  has less stamina left.
- **Wall-walking needs height:** the arena or map needs walls and ceilings that are worth
  using.
- **Deferred:** eating/absorbing as a source of abilities – too complex for now. Keep it for
  later, e.g. as a body skill (slimes that absorb the abilities of others).

### Auras: howl and roar

- A voice skill is not a button with a cooldown but an **aura**: switched on, it **binds a
  fixed amount of stamina** for as long as it stays on (like an augment binds mana) and
  strengthens **allies within a radius** (guess: about 15 m). Switched on and off via the
  outer ring of the magic wheel, with the short moment of binding.
- **Stamina, not mana:** the voice is physical, and wolves as endurance hunters have more
  stamina than mana. The price is felt: less stamina for one's own attacks. Augments stay
  on mana – each bar binds its own kind of passive.
- **The moment stays:** switching on is a visible and audible howl (the telegraph); then the
  aura runs quietly with a visible shimmer or breath effect – opponents can see whom to take
  out first.
- **No stacking:** of each kind of aura only the strongest counts, so a pack of five howlers
  does not outshine everything.
- A higher skill level gives **more effect** (maybe a little more radius), not less binding.
  Bound stamina × time counts as use for the hXP, so wearing the aura in a fight trains it.
- **Leader auras** (from the practice skill leadership, see
  [progression.md](progression.md#leadership)) follow the same rules but work only while
  the character leads the group. A howl and a leader aura are different kinds, so both
  count.
- **Allies only, for now.** A roar against enemies (intimidation) is a possible later variant,
  e.g. for dragons.
- The pack wolf's howl: allies nearby regenerate stamina faster; the alpha wolf's howl works
  on the whole group in a larger radius.

### Order of the lines

- How many lines there will be will become clear over time. They are added one by one:
  simple ones first, to learn from them; complex ones (spiders, dragons with legendary
  forms) later.
- Criteria for the first line: on the ground (no flight, no wall-walking), two to three
  forms, clear body skills, covers one branch.
- **Proposal: slime → saurian → wolf** (trees below). The slime is the cheapest to animate
  and tests the element system. The saurians show visible body change with one shared
  quadruped rig and cover armored and colossus. The wolf delivers the agile branch that the
  first two lack.
- After that, roughly: lines with gliding or flight → spiders (wall-walking, threads) →
  dragons and colossi with legendary forms.
- Idea for later (not at launch): **cats** with a focus on stealth.
- Only part of each tree is built at first (two to three forms per line). The rest follows
  step by step.

---

## First lines (proposal)

Every form has a rarity, a level cap and a predecessor (see evolution in
[progression.md](progression.md); guide values for the caps under character level in
[progression.md](progression.md)). "Common" is the rarity of normal forms.

### Slime

Being rebuilt – draft, details to follow.

- **Traditionally very weak:** the starting form "slime" has very low values.
- **Whole tree:** raised resistance to physical damage (soft bodies).
- **No affinity** – no element is easier or harder.
- **Attacks:** a slam and a **projectile** (body skill, base skill of the slime). Both start
  without element. With an element imprint, the projectile takes on the element. With
  several imprints, the element can be switched (via the magic wheel).
- No rubber jump.
- **Two branches that exclude each other:** the element branch and the physical branch.

#### Branch 1: elements

First one element, then more and more. On each evolution the branch takes along its
**highest imprints** and resets them to level 1. Which are the highest is decided by level
and collected XP; on an exact tie, the player chooses.

| Form | Rarity | Unlocked by | Takes along | Property |
|---|---|---|---|---|
| **Slime** | common | start | – | very weak, physically resistant; base skill projectile |
| **Elemental slime** | common | first element imprint | the highest imprint | **element body:** melee hits against it deal some elemental damage back to the attacker |
| **Chimera slime** | common | second element imprint | two; three if the third has reached at least level 3 | switches element faster than others |
| **Prism slime** | epic | 4 element imprints | all | may learn **opposing imprints** |
| **Primordial slime** | legendary | 2 opposing pairs (e.g. fire and ice, lightning and earth) | none – see below instead | see below |

- Spell forms are not tied to the form but to the skill of the element in question (see
  [magic.md](magic.md)).
- **Primordial slime:** all imprints go to 0. In return it gets a **high affinity for all
  elements** (not physical) – as an exception, even for opposites at the same time. A fresh
  start with full freedom: at first almost bare elementally, but with the greatest
  potential.
- **Primordial slime without the milestone limit:** the rule "one imprint per milestone"
  does not apply to it. It develops imprints as soon as its progress is enough.
- **Smaller penalty for several elements:** for the element branch the threshold for further
  imprints rises more gently, and the weakening from several elements gets smaller with
  each form – for the primordial slime it is 0.
- The values for X are provisional. Whether rare forms are hard enough but not impossible
  must be shown by a test (probably in the live game).

#### Branch 2: physical values and size

Unlocked through values such as strength and a **physical imprint**. Only for the slime
there is an alternative: reaching its level cap without an element imprint. This way every
slime can evolve at least once.

| Form | Rarity | Unlocked by | Body skills | Property |
|---|---|---|---|---|
| **Giant slime** | common | physical imprint level 1 and minimum strength – or (slime only) level cap without element imprint | base: projectile, **stomp**; accessible: **charge** (it rolls over others) | big, a lot of HP, hard to interrupt |
| **King slime** | epic | **highest level** of the physical imprint, high strength and vitality (the level budget was dropped on 2026-11-13) | stomp, charge | colossus; **gel burst:** a physical wave that slows you while you are in its field |

- No legendary form in the physical branch for now.

#### Level caps and mimic shape

- **Level caps:** slime 30 (very low – traditionally the weakest monster), elemental slime
  50, chimera slime 100 (rare), prism slime 120, primordial slime 150; giant slime 100
  (rare), king slime 120.
- **Mimic shape** (from epic forms on, e.g. prism, king, primordial slime): a passive skill
  that can be switched on. Thanks to their changeability, slimes can deceive humanoid races.
  In humanoid form they are seen as one of them instead of as a Shifter (see cities,
  relations and zones in [world.md](world.md)).

### Saurian

Based on four-legged dinosaurs, with fantasy names.

- **A big animal from the start:** a high starting form, followed directly by rare forms –
  it does not transform often.
- All forms share one quadruped rig; only model and proportions change.
- **Affinity:** high for earth, low for lightning (opposing element). **Perception:**
  rather low.
- Deliberately simple for the start: three directions, only the longneck goes further.
  Can be extended later (e.g. a plated saurian).

| Form | Rarity | Cap | Evolves from | Condition | Body skills |
|---|---|---|---|---|---|
| **Young saurian** | common | 80 | – (starting form) | – | base: **charge**; accessible: **tail strike** |
| **Horned saurian** | rare | 100 | young saurian | high strength and fire imprint level 4 or higher (deliberately against the earth affinity – a fire horn) | charge, **front shield** (neck frill; opponents look for the flank) |
| **Armored saurian** | rare | 100 | young saurian | high physical resistance and earth imprint level 5 | **tail strike** as a club to the rear; accessible: **stomp** |
| **Longneck** | rare | 100 | young saurian | high vitality or level cap reached (physical form) | tail strike as a whip, **stomp**; colossus with a large HP pool |
| **Titan** | epic | 120 | longneck | high vitality, earth imprint level 5 and a second element of free choice at level 3 or higher (not lightning – opposing element) | **stomp** as a large area attack with both elements; tail strike |

- **Titan:** at its core an earth colossus. The second element adds variety (fire, ice,
  poison titan …). Property of the form: with **stomp**, **both elements apply in full** – a
  targeted rule-break for this attack only. Otherwise the weakening for several elements
  applies.
- Armored saurian and titan both demand high earth imprints. That is acceptable, because
  they sit in different branches and at different points in the tree.

### Wolf

Endurance hunters.

- **Role:** the wolf wins because it lasts longer and wears its prey down – not through
  short sprints or stealth. In return it is not especially tough and is easy to interrupt
  (small HP pool).
- **Affinity:** high for ice and wind; low for fire. **Perception:** high (tracker). All
  forms share their own slim quadruped rig.
- **Two directions:** physical (with a branch for buffs) and through the elements ice and
  wind.

| Form | Rarity | Cap | Evolves from | Condition | Body skills |
|---|---|---|---|---|---|
| **Wolf pup** | common | 40 | – (starting form) | – | base: **pursuit**, **pounce**; accessible: **wear down** |
| **Wolf** | common | 60 | wolf pup | level cap reached or certain physical values | base: pursuit, pounce, wear down; accessible: **howl** |
| **Grey wolf** | rare | 100 | wolf | certain physical values | like wolf; stronger wear down, reserve barely drops – the long-distance hunter |
| **Pack wolf** | rare | 100 | wolf | certain buff skills (e.g. howl) | wear down, **howl** (aura): allies nearby regenerate stamina faster |
| **Alpha wolf** | epic | 120 | pack wolf | certain buff skills | howl (aura) affects the whole group in a larger radius; allies wear down too; leadership could be a requirement (bridge to the leaders) |
| **Frost wolf** | common | 60 | wolf pup | ice imprint level 3 | base: pursuit, pounce; accessible: wear down; ice focus |
| **Windrunner** | common | 60 | wolf pup | wind imprint level 3 | base: pursuit, pounce; accessible: wear down; wind focus |
| **Polar wolf** | rare | 100 | frost wolf or windrunner | ice and wind imprint, together at least level 3 | pursuit, pounce, wear down; ice and wind focus |
| **Fenrir** | legendary | 150 | polar wolf | lightning imprint level 4, ice and wind each level 3 | all; comparatively big, but **not a colossus** – focus on speed and endurance. Rule-break: **unstoppable** – slow, chill, heaviness and headwind have no effect. Lightning gives short, lightning-fast sprints |

- The imprints for polar wolf and Fenrir are each developed anew in the current form
  (imprints are reset on evolution). The raised affinity from earlier imprints helps.
  Lightning is not part of the wolves' affinity – for Fenrir a real detour.
- The element branch goes from the rare polar wolf directly to the legendary form, with no
  epic form in between.
- **Role next to the other lines:** slimes take a lot of physical punishment; wolves last
  longer. Counter: put the wolf under pressure early, before wear down takes effect. Fenrir
  counters ice and earth magic that targets movement – that is why it is legendarily rare.

### General

- Use permanent fields around a character (e.g. king slime) sparingly, so they stay
  something special.

**For the arena (next step):** slime → fire elemental slime against a humanoid (an opponent
with ice tests the opposing pair). After that, young saurian → horned saurian or armored
saurian.

---

## Legendary forms (monsters)

- Not everyone reaches every evolution. Some forms – not necessarily for every monster –
  exist only if you keep seeking very hard fights or level under harder conditions (high
  hardship rating).
- **Legendary forms** are meant to be reached by very few players.
- **Rarity:** extremely rare in the MMO – just meeting a legendary form should be something
  special. In the arena and on fixed maps (stage 1 and 2) it may happen more often.

## Counterpart for humanoids

- **Hidden classes and titles** (chosen approach): classes you do not pick but earn – e.g.
  "Dragonslayer" only for someone who has defeated a dragon far above their own level.
  Legendary classes and titles are as rare as legendary monster forms.
- **Two kinds of titles:** skill titles (from skill levels, e.g. *Swordmaster*, *Ice Mage*)
  and hidden titles (from special achievements, e.g. *Dragonslayer*). **All titles always
  apply at the same time.** So that this does not escalate, multipliers from titles should
  be small and add up instead of multiplying (details in balancing).
- **Skill titles and traits instead of skill upgrades:** skills simply keep levelling; every
  30 levels a title (percentages), in between traits to choose from (properties) – see
  skills under progression in [progression.md](progression.md).
- Rejected: letting the species itself rank up (elf → high elf …).

## Humanoid form for monsters

- From a certain, late point on, monsters can take on a humanoid form. Its look is designed
  in the character editor the first time (see [Shifters: the editor again](#shifters-the-editor-again)).
- In this form, stats and skills are weakened as long as you wear it – except for legendary
  forms or similar.
- Transformed monsters always count among humanoids as their own humanoid race, the
  **Shifters** – like the relation between human and elf, not as monsters. Exception:
  slimes from epic forms on have the capability **mimicry**: NPCs do not treat them as
  Shifters (no distrust or access limits for Shifters in cities). They still **look** like a
  Shifter with slime marking, so other players recognise them – only NPCs are fooled. (Later,
  optionally: NPCs with high perception see through it.)
- The humanoid form opens doors: trade, quests, access to cities you could never enter in
  monster form – at the cost of combat strength (see cities, relations and zones in
  [world.md](world.md)).
- In humanoid form, monsters can also **wear armor**. The balancing takes the stat loss of
  the form into account.

---

## Capabilities

**Capabilities** are fixed abilities of a form or race – passive, they **do not level**.
Each form and race has its **own list** (separate from the catalogue of body skills, which
is for monsters only; humanoids have capabilities too).

- A capability **unlocks an action**. Without it, the action does not exist for that form.
- **First capabilities:** **dodge**, **jump**, **crouch**; from the body-skill catalogue
  **pursuit**, **front shield**, **glide/fly** and **wall-walking**; **mimicry** for slimes
  from epic forms on (see [Humanoid form for monsters](#humanoid-form-for-monsters)).
- **Most species have dodge, jump and crouch.** Gigantic forms have none of them – they are
  such a large target that dodging would hardly help; their counter is a large HP pool and
  high resistances. Slimes need no crouch (they are small anyway).
- **How well** an action works comes from attributes, not from the capability: dexterity
  sets dodge distance and speed and jump height (see [progression.md](progression.md)).
- **Alternatives replace the standard** (later): e.g. a dodge roll or a short-range teleport
  instead of the normal dodge step. A form has at most one variant per action; evolutions
  can swap it.

## Character creation

- **Short, quickly into the game:** species or race → look → name, then the tutorial. The
  real choices (weapon, element, skills) are made **by playing in the tutorial**, not in
  menus.
- **No stats and no class at creation.** Strength, dexterity and so on are earned in play;
  the play style grows out of playing.
- The earlier **starting alignment** (an element or elementless) is dropped: the free skill
  point in the tutorial can unlock an element skill instead (see
  [progression.md](progression.md#unlocking-skills-teachers-and-learning-by-use)).

### Tutorial

- A **weapon rack** with wooden starter weapons of every family (rarity raw): you try them,
  feel the learning phase (+100 % demand at skill level 0), and a **teacher** there unlocks
  your first skill with the first free skill point – a weapon or an element. This explains
  learning phase, free points and teachers along the way.

### Look of monsters: marking and colors

- The model changes with every evolution, but the **marking stays**: a base color and a
  pattern (spots, stripes, eye color) chosen at creation are carried over to every new form.
  This is how a player recognises "their" slime, wolf or saurian.
- **Imprints unlock colors** (e.g. ember red after the fire imprint) – a visible sign of the
  character's history. The imprint's own tint comes on top; the marking stays.
- Technically cheap: marking and colors are texture or material parameters, not models.
- Monsters have no gender in the model.

### Look of humanoids: sliders, gender-fluid

- **Target look (wish of the human, 2026-11-20): stylized, in the direction of Palia** –
  instead of the realistic MakeHuman body of M5. The proposed path keeps MakeHuman as the
  technical base and stylizes it through its **proxy system** (see open questions); it is
  tested before anything is bought or commissioned.

- The body is shaped with **sliders**. There are no fixed male and female models and no
  model per race: **one shared humanoid base model** with one skeleton for all races
  (Shifters included), shaped by shape keys (morph targets). Three **starting presets** –
  classically male, classically female, neutral – are just slider positions on that model.
- **Races = slider limits plus add-on parts:** each race has its own ranges (dwarves short and
  stocky, elves slim), its own skin colors, and add-ons such as elf ears, Demon horns or a
  tail as extra shape keys or attachable parts.
- **Bone lengths too:** strong proportion changes (a dwarf's shorter legs, broader torso)
  need some sliders to scale **bone lengths**, not only shape keys – otherwise the joints sit
  wrong. Unreal does this at runtime (bone transforms in the C++ anim instance); retargeted
  animations adapt. The range stays limited (dwarf yes, giant no); bodies with wings or four
  legs are not humanoids anyway.
- **One rig, one set of animations and one set of armor models for all races.** Animations
  are shared, so a dwarf moves like a human at first; race-specific stances (idle, gait) can
  be added later where they matter.
- **Look is separate from gameplay:** the shape sliders are purely cosmetic and change
  neither hitbox nor stats. Only the **size slider** (body weight, see below) affects play.
- **Armor follows every body shape:** the shape keys (and bone lengths) are transferred from
  the body to each armor model by script (Blender data transfer or surface deform) – the
  main extra effort, solved in the pipeline, and only once for all races. One armor model
  instead of one per gender roughly halves the work on armor and clothing; fitted shapes are
  added only where wanted (see body shapes in [items.md](items.md#armor-and-body-shapes)).
- **Voice** (or pitch) is chosen separately from the body; if NPCs address the character,
  **pronouns** are chosen too (or NPCs use the name).

### Shifters: the editor again

- When a monster gets the humanoid form for the first time, the **same editor** opens for its
  humanoid figure – the shared base model within the slider limits of the **Shifter** race.
  Slimes with mimicry use the same model; they are not built as other races.
- **The marking carries over:** base color, pattern and unlocked colors appear on the
  humanoid figure too (hair, eyes, skin markings), so the monster stays recognisable.
- **Only in a safe moment:** the first transformation – and with it the editor – works only
  out of combat with no enemies nearby (like resting at the camp or nest), so nobody stands
  helpless in the open world while editing. The ability can wait until then.
- The size slider (body weight) applies to the Shifter figure as for other races.

### Size (humanoids)

- Size can be adjusted and works through **body weight** – not through values such as
  strength or dexterity (those are earned in play):
  - **Tall = heavier:** a little more reach; a weapon weighs a little less relative to the
    body, so strikes cost slightly less stamina. In return, dodging costs more, you turn a
    little more slowly, you are noticed from a little further away, and the hitbox is
    bigger.
  - **Small = lighter:** dodging costs less, you turn a little faster, you are noticed later,
    harder to hit. In return, a little less reach and slightly costlier strikes.
  - Size no longer affects interrupts (they depend on a share of maximum HP).
- **Limited range per race** (e.g. ±10 %), so that the hitbox difference stays small and
  does not become a must-have advantage.
- **Races have different base sizes** (e.g. dwarves smaller). This is balanced through
  affinities and base values.
- Monsters: their size comes from the form.

## Unlocking species

- **A mix of open and unlockable.**
- **Open from the start:** **humans** and the **slime** as the first monster. The monster
  fantasy is the unique selling point and so is never behind a lock. Among the humanoids
  there is still something to unlock.
- **Further races and monster starts** (elves, Demons, wolf pup, young saurian …) are
  unlocked with an **in-game currency** (the unlock currency), earned through achievements
  and daily and weekly tasks. Unlocks apply account-wide.
- **Not account-wide:** forms, classes and titles apply only to the character who reached
  them. No strength carries over between characters (instincts, imprints).
- **Everything open only in the alpha** (arena) and for test accounts. After that, unlocking
  applies there too.
