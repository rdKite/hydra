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
  - protection against slash, pierce and blunt;
  - element properties (metal conducts lightning and heats up in heat or cools down in cold;
    fur and cloth insulate against cold …);
  - a **density** (weight per thickness).
- **Thickness** sets how high the protection is and how much the armor weighs. It is part of
  the schematic (e.g. "light breastplate" and "heavy breastplate") and cannot be set freely.
  Example: metal protects strongly against slash and pierce, but is heavy and weak against
  blunt and lightning.
- **Monster material inherits the being's affinity or imprint:** scales of a fire armored
  saurian protect against fire, gel of an ice slime protects against cold. This way humanoids
  "borrow" properties from monsters – the counterpart to evolution. Rare monsters provide rare
  material; this links hunting, crafting and the economy.
- **Material only from NPC monsters**, not from defeated players: whoever respawns does not
  really die and leaves no corpse.

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
- **Cap** on protection: armor reduces damage by at most a fixed share. No meat shields.
- **Visible:** material and rough thickness can be recognised, so the opponent can adapt
  weapon and element (guiding principle, see [overview.md](overview.md)).
- **Weight works both ways:** armor costs stamina when moving, but makes you harder to
  interrupt (see [combat.md](combat.md)).
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
  through an augment" still holds (see [magic.md](magic.md)). Other bonuses: e.g. speed,
  block strength, lower stamina costs.
- Hilts also have fixed recipes in different rarities.

### Rarity in crafting

- One term for raw materials and items: **rarity** (levels see [Rarity](#rarity)). Each level
  raises the fixed values by a factor.
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
  difference per level gives a clear advantage, but no doubling (guide value rather +10–15 %
  per level). Even the rarest items stay visible and counterable.
- **By stage** (proposal): the arena with fixed equipment sets, without crafting or rarity;
  fixed maps with crafting and masterworks; unique items in the MMO, mythic ones there only
  after launch.
