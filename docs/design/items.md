# Items: armor, crafting and rarity

This document covers armor, crafting of weapons and armor, and rarity. Weapons themselves
(weapon families, weapon properties) are in [combat.md](combat.md). Terms follow
`GLOSSARY.md`.

---

## Armor

There are no classic armor classes from light to heavy. Armor follows from **material and
schematic** – the same logic as for the weight of weapons and movement. Armor is made from
components, like weapons (see [Crafting](#crafting-weapons-and-armor)).

### Material and thickness

- **Every material has a profile** (fixed values) per thickness:
  - protection against slash, pierce and blunt, as **resistance points** (they add to the
    wearer's points, see resistances in [progression.md](progression.md#resistances));
  - element properties (metal conducts lightning and heats up in heat or cools down in cold;
    fur and cloth insulate against cold …).
- **Weight belongs to the schematic, not to the material** – for weapons and armor alike.
  Every craft has its own schematics, so the difference comes by itself: a plate harness
  (armor smithing) is heavy, a leather jerkin (leatherworking) light, a robe (tailoring)
  very light. Within a craft the material changes protection and look, not weight: a mithril
  harness weighs as much as an iron one but protects better. Fantasy materials therefore
  need no invented weight.
- **Thickness** sets how high the protection is and how much the armor weighs. It is part of
  the schematic (e.g. "light breastplate" and "heavy breastplate") and cannot be set freely.

  | Craft | Example schematic | Full set, light / heavy |
  |---|---|---|
  | Tailoring | robe, gambeson | 2 / 5 kg – little protection, very quiet |
  | Leatherworking | leather jerkin | 5 / 8 kg – scouts |
  | Armor carving | scale, bone, chitin armor | 8 / 14 kg |
  | Armor smithing | mail / plate harness | 12 / 25 kg – much protection, heavy, loud |

  Split over the slots: torso 50 %, legs 20 %, head 10 %, boots 10 %, hands 5 %, back 5 %.
- **Protection targets (guess, blunt points of a full set at a matching level):** robe ~20,
  leather ~60, scale or bone ~90, plate ~70 against blunt (metal is weak against it) but
  ~150 against slash and pierce. Target: medium armor survives an uncharged heavy hit of
  the heaviest weapon of an equal opponent to the front (needs about 55 points); a fully
  charged one needs about 130 (see the guard rail in
  [combat.md](combat.md#high-damage-no-meat-shields)). For monsters the base resistance
  points of the form take this role.
  Example: a human in heavy plate weighs 95 kg – dodging costs × 1.36, turning and sprinting
  slow down, and they are noticed from further away.
- Example: metal protects strongly against slash and pierce, but is weak against blunt and
  lightning.
- **Monster material inherits the being's affinity or imprint:** scales of a fire armored
  saurian protect against fire, gel of an ice slime protects against cold. This way humanoids
  "borrow" properties from monsters – the counterpart to evolution. Rare monsters provide rare
  material; this links hunting, crafting and the economy.
- **Material only from NPC monsters**, not from defeated players: whoever respawns does not
  really die and leaves no corpse.

### Armor and body shapes

One base model for every body (see character creation in
[monsters.md](monsters.md#look-of-humanoids-sliders-gender-fluid)) means one armor model
for every body too. Rules:

- **Soft clothing follows the body fully** – cloth, leather, robes, gambesons take on the body
  shape through the shape keys (or as proxies). A rather female body gets fitted clothing by
  itself, with no extra work.
- **Standard silhouette:** armor has a silhouette that looks good on every body.
- **Rigid torso pieces** (breastplate, plate harness) get an extra **fitted shape key** that
  follows the body's gender slider – no second model. Agents make it from the body shape by
  script and smooth it so metal stays rigid instead of rubbery; an artist may refine visible
  pieces. Arms, legs, helmet and boots hardly need it.
- **Fitted schematic variants** for selected armor, e.g. a *tailored harness* next to the
  *straight harness*, with their own silhouette and design – **open to everyone** as a style
  choice, not locked by gender. One extra model per variant, only where it is wanted.
- **Check matrix:** every new armor or clothing piece passes automatic pictures at the corners
  – the three body presets × slim / heavy × human / dwarf – before it goes in.

### Components and schematics

- **Two components:** a **base layer** (material, main values – like the blade) and a
  **lining or fittings** (bonuses – like the hilt).
- **Schematics set the shape** (armor type, slot, thickness); you choose the material yourself.
  One schematic is one model; the material changes the look through colour and effects. This
  keeps the number of 3D models limited.
- Merchants sell typical armor ready-made.

### Slots

- **Head, torso, hands, legs, boots, back** (back as a proposal: back piece or cloak). The
  thickness can differ per slot (through the schematic) – e.g. thicker at the back, thinner on
  the arms.
- **Weak points:** protection at the back lowers the weak point multiplier, but never removes
  it fully – light weapons stay interesting against armored opponents too.

### Guard rails

- **Diminishing protection, linear weight:** each thicker step of a schematic gives a little
  less extra protection, but adds the full extra weight. This gives a natural optimum instead
  of endless stacking.
- **No hard cap** on protection: armor adds resistance points, and the flattening curve of
  the resistances makes every further point worth less. No meat shields.
- **Visible:** material and rough thickness can be recognised, so the opponent can adapt
  weapon and element (guiding principle, see [overview.md](overview.md)).
- **Weight has a price:** heavy armor makes dodging costlier, turning and running slower,
  and you are noticed from further away (see stealth and perception in
  [combat.md](combat.md#3-stealth-and-perception)). It does not make you harder to
  interrupt (interrupts depend on a share of maximum HP). Light armor makes a scout.
- **Armor does not wear out.**
- **No separate skill for wearing armor:** strength and endurance grow anyway by carrying heavy
  loads and moving with them – a second skill would duplicate that.
- Protection against elements comes mainly from defensive augments and imprint (see
  [magic.md](magic.md)). Armor is physical at its core, with the element properties of its
  material.

---

## Crafting (weapons and armor)

Not very complex, but versatile: items are assembled from **components**, and each component
can be made in different ways.

### Components

- **Standard: two components** per item.
  - Weapon: **blade** (or head, tip, limbs) for the main values and **hilt** (or shaft) for
    bonuses.
  - Armor: **base layer** and **lining or fittings**.
- **Special equipment** (later) can consist of more components – with minimum requirements on
  materials and a minimum rarity.
- **Schematics per component** are created individually; together they still give a large
  number of different weapons and armor pieces. The 3D models are plugged together; the
  material only changes the look.
- **Example:** a copper blade (low values) plus a hilt with a fire bonus gives a
  **"Copper Longsword of Fire (common)"**. The name explains the weapon, also to the opponent
  (guiding principle, see [overview.md](overview.md)).

### Materials

- **Built-in materials with fixed values:** directly mineable metals (copper, iron …), alloys
  (steel …) and monster materials (dragon bone …).
- **Alloys** also have recipes. Real smithing techniques (Damascus steel, a blade made of core
  and edge …) come later as recipes within this system, not as a separate mechanic.
- **Monster material keeps its affinity:** a blade made of dragon bone carries the dragon's
  element in its values, e.g. as a bonus to the matching augment.

### The hilt gives bonuses

- For example a percentage boost to one element ("of Fire"): this strengthens the **fire
  augment** by X %. It is not separate element damage – the rule "element on weapons only
  through an augment" still holds (see [magic.md](magic.md)). Other bonuses: e.g. damage,
  chain, block arc. Hilt bonuses add to the pool of progress bonuses (see damage in
  [progression.md](progression.md#damage)); they do not lower stamina costs.
- Hilts also have fixed recipes in different rarities.

### What makes a weapon better

| Lever | Better through | Price |
|---|---|---|
| **Size** (schematic) | more damage, more reach | more weight: stamina, strength requirement, turning, being noticed |
| **Material** | more base damage, its profile (slash, pierce, blunt); affinity (monster material) | rare deposits; a material strong in one damage type is weaker in another – weight does not change |
| **Rarity** | **+12 % damage per level** | **+6 % demand per level**; effort in crafting |
| **Hilt** | bonuses into the pool of progress bonuses (added up) | – |
| **Masterwork** | one special property | from rare on only, high effort |

- **Demand:** a factor on a weapon's stamina cost, on top of weapon weight ÷ body weight –
  how much force and control the weapon asks for. It slows the number of strikes in a row
  that grows with the bar (see [magic.md](magic.md#stamina-and-mana)). A better weapon is
  still always a gain, because damage rises faster than demand. While the weapon family's
  skill is still at level 0, demand is **+100 %** (learning phase, see
  [progression.md](progression.md#unlocking-skills-teachers-and-learning-by-use)).
- **Rarity works the same way on armor:** each level gives armor +12 % resistance points,
  so better weapons do not shorten fights on their own.
- **Requirements are soft:** no locks by level or stats. A weapon too heavy for your
  strength is simply harder to handle (turning, drawing, drawing a bow). Light weapons can
  be used by anyone; dexterity only makes them better.
- Compared: Guild Wars 2 raises weapon strength by about 8 % per rarity level (fine to
  ascended) and keeps legendary equal to ascended. hydra keeps a steady +12 % instead.

### Crafting skills

Six crafting skills, by material and use:

| Skill | Makes |
|---|---|
| **Weapon smithing** (`weapon_smithing`) | weapons of metal, including metalworking |
| **Armor smithing** (`armor_smithing`) | armor and shields of metal, including metalworking |
| **Weapon carving** (`weapon_carving`) | bows, shafts, clubs, bone blades, hilts – wood and hard monster materials |
| **Armor carving** (`armor_carving`) | wooden shields; bone, horn, chitin and scale armor |
| **Leatherworking** (`leatherworking`) | leather armor; soft monster materials (hide, gel) |
| **Tailoring** (`tailoring`) | cloth armor, robes, linings |

- **Every material has a processing type** in the material data: metal, hard (wood, bone,
  horn, chitin, scales), soft (leather, hide, gel) or cloth. It decides which craft works
  it – also for monster material.
- **Every component is made by its own craft:** a steel blade by the weapon smith, a wooden
  hilt by the weapon carver, a leather lining by the leatherworker. **Assembling** is done by
  whoever has the craft of the main component (blade or base layer). This makes trade
  between players worthwhile; solo players buy hilts and linings from merchants.
- **Shields belong to the armor side** (armor smithing or armor carving).
- Gathering (ore, wood) has no skill; it trains attributes.

### Rarity in crafting

- One term for raw materials and items: **rarity** (levels see [Rarity](#rarity)). Each level
  raises the fixed values (see [What makes a weapon better](#what-makes-a-weapon-better)).
- **The rarity of a component** follows from the rarity or purity grade of its materials.
- **Skill as a gate:** a minimum skill is needed to be able to make materials (e.g. alloys) and
  components of a given rarity at all. No randomness – every result is predictable.
- **Same rarity to assemble:** all components of an item must have the same rarity.

### Rarity of raw materials

- **By location:** hard dungeons, tough enemies that drop it, or simply a rare deposit.
- **By a catalyst:** raw materials of lower rarity can be upgraded with a rare catalyst.
  (proposal) A **monster core** from strong enemies – so it pays off for crafters to seek out
  hard fights too.

---

## Rarity

| Level | What | Source |
|---|---|---|
| **Raw** | the absolute basics: made of one material, no extra values | simple merchants, easy to make |
| **Common – rare** | items from normal schematics (two components) | crafting |
| **Rare – legendary** | **masterworks:** special schematics, more components, minimum requirements on material and rarity; legendary ones need considerable effort in resources and time | crafting |
| **Mythic** | cannot be crafted | earned only extremely rarely through specific events – only in Phase 3, and not at its launch |
| **Unique** | one of a kind worldwide; not necessarily weapons | only in the MMO part |

- Order: raw → common → uncommon → rare → epic → legendary → mythic → unique.
- **Legendary equipment in the MMO (Phase 3):** unlocked like epic and legendary forms there –
  achievements plus large amounts of resources, similar to the second-generation legendary
  weapons in Guild Wars 2. Fine-tuning only with experience from Phase 2.
- **Masterworks** get a **special property**, not just higher values – like traits, but
  without overlapping them (e.g. a sword whose block costs less stamina; an armor that
  dissolves element fields faster).
- **Unique items** can mean more than combat power – e.g. make a player ruler over a territory
  (bridge to the strategy part, see [strategy.md](strategy.md)). They can change owners, but
  only under **very special conditions per item** – not simply by defeating the bearer
  (otherwise individuals get hunted).
- **Noticeable but beatable** (guiding principle, see [overview.md](overview.md)): the
  difference per level gives a clear advantage, but no doubling (+12 % damage or armor
  points per level, see [What makes a weapon better](#what-makes-a-weapon-better)). Even the
  rarest items stay visible and counterable.
- **By stage** (proposal): the arena with fixed equipment sets, without crafting or rarity;
  fixed maps with crafting and masterworks; unique items in the MMO, mythic ones there only
  after launch.
