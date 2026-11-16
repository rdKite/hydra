# Strategy layer (Phase 3, MMO)

Inspired by Crusader Kings, but much simpler. Terms follow `GLOSSARY.md`. Back to
[overview.md](overview.md); relations, reputation and zones are in [world.md](world.md).

## Territories and rule

- The world is divided into **territories** – each with a settlement, defense towers,
  resource deposits and possibly a dungeon. At first all of them belong to NPC factions;
  players or guilds can take them over.
- **Rule depends on a unique item** (the crown or seal of a territory). It belongs to one
  person; the power rests on a **guild**.
- **Change of ownership** only under special conditions, e.g. a won war, a succession the
  ruler chose, or **inactivity** (the seal passes to a successor – absent rulers block
  nothing).
- **Hierarchy in levels** (fiefs): likely several levels as in Crusader Kings (e.g. county
  → duchy → kingdom). How many depends on the size of the map and the number of
  territories – decide this when the world is planned.

## Decisions of rulers (few, but powerful)

- Taxes (e.g. on trade), the stance of the guards towards races, species and factions,
  laws (PvP rules of the zone, bounties), building (towers, walls, inn, workshops),
  diplomacy (alliance, war, peace), appointments (stewards, commanders).
- **Satisfaction of the population:** high taxes and lack of safety make people
  dissatisfied – up to uprisings (NPC rebels).

## Control of a territory

- One value per territory, driven by wealth, certain buildings and the satisfaction of the
  population.
- **Control decides how often patrols are out** – and thus how dangerous the territory is
  for untransformed monsters. Well-controlled core territories are very hard for monsters;
  poorly controlled border regions are easier.

## Changing world: places with states

- The **landscape itself stays unchanged** (no craters or similar). Buildings and
  environment switch between **fixed states** and recover again.
- **Fixed building sites** instead of free placement – each place has a type and a
  sequence of states, e.g.:

  | Place | States |
  |---|---|
  | Field or farm | forest → clearing → field or farm → abandoned → overgrown → forest |
  | Outpost or tower | building site → under construction → intact → damaged → destroyed or ruin → rebuilt |
  | Settlement | grows or shrinks in steps; houses stand empty or become ruins |

- **Population, control, war and time** decide the state. Changes happen gradually (e.g.
  over days). Fields feed the population. Through war or a shrinking population they are
  destroyed or abandoned, and the forest takes the land back.
- **Cover changes too:** if a city clears the forest around it, there are fewer objects
  attackers can sneak behind (but the wood is gone). Grain fields give new cover; an
  overgrown farm becomes a hiding place (purely visual, see stealth and perception in
  [combat.md](combat.md)).
- **Some choice per place:** depending on its type, a building site can carry different
  buildings (e.g. forest site: woodcutter, or clearing for a field; ore deposit: mine).
- **Tiers** as a second axis next to the state: per place, the building type, the
  **tier** (e.g. 1–3) and the **state** count. Higher tiers give more yield or more
  toughness (towers, walls), but cost more to build and maintain (fits overextension) and
  need enough control or population. War can push tiers back; abandoned buildings lose
  tiers over time until nature takes over.
- **Graphics:** each tier has its own model. States such as damaged or abandoned are done
  as overlays (damage, overgrowth) where possible, instead of a model per combination.
- **Why fixed places:** per place only a few small numbers need to be stored and sent
  (instead of many single objects); every state is designed by hand; resources per
  territory can be planned; nobody can build paths shut.
- **Free building** exists only in the survival part – at your own camp or nest, with
  limits.

## Economy

- Territories supply resources of different rarity. Taxes pay the upkeep (NPC units,
  towers, upgrades).
- **Against oversized realms:** upkeep rises more than proportionally with each additional
  territory (overextension) – no guild can hold the whole map.

## War

- **Declaration of war with a lead time** (e.g. 24 hours) and **fixed time windows** for
  battles over territories – no attacks when nobody is online.
- NPC units march against the towers as in a MOBA, and players fight along. Whoever takes
  the towers and the center wins the territory. During the war there are no reputation
  penalties against the war enemies.
- **Advantage for the defense** through towers and walls, so small guilds have a chance.

## Leaders

- **Following:** players can follow a person or swear loyalty to them. The number of
  **active** followers unlocks abilities for leaders – so leadership is "granted by the
  community", also for streamers, without any link to external platforms.
- Abilities, e.g.: **banner** (buffs for the group in war), **orders to NPC units**, more
  fiefs. Leader can be a hidden title.

## Coats of arms and colors

- **Design only:** whether someone is friend or foe is shown by classic indicators (green,
  yellow, red – friendly, neutral, hostile), not by coats of arms or colors.
- **Who sets them:** the guild that holds rule over a territory.
- **Where you see them:** on armor, shields, banners, buildings and boundary stones – and
  on the **strategy map**, which shows which territory belongs to which faction.
- Proposals:
  - **Coats of arms from building blocks** (proposal) (shield shape, division, colors,
    charge) – stored as data, so they cost little storage and bandwidth and can be
    extended by mods.
  - **Fiefs** (proposal): vassals show their own coat of arms, plus the colors of the realm
    they serve (as in Crusader Kings).
  - **Colors do not cover material and imprint** (proposal): faction colors go on cloth,
    banners and coat-of-arms areas; material (metal, scales) and imprint stay
    recognizable.
  - **Monsters** (proposal) show their allegiance e.g. through body paint or a banner or
    collar; in humanoid form like everyone else, through their armor.

## Monsters in the strategy layer

- Monsters rule only as **Shifters** or through territories of the **Demons**.
- Untransformed monsters are mainly an **outside force**: they can enter any territory,
  including ones controlled by other factions – as a threat whose danger is set by the
  territory's control (patrols).
