# World

How players, monsters and NPCs relate to each other in the world: friendly fire,
reputation, cities and zones, dungeons. Also the world-level topics monetization, look,
the three expansion stages and mods. Terms follow `GLOSSARY.md`. Back to
[overview.md](overview.md).

## Friendly fire and reputation

Three layers that together also cover the case humanoid vs. monster:

1. **Technical (server setting):** friendly fire in groups, guilds and alliances is off by
   default. PvP can be switched on and off globally or per zone.
2. **World (reputation per faction or species):** whoever attacks members of their own or
   an allied faction loses reputation there, is marked as an offender, gets a bounty and is
   hunted by guards. A human who attacks a monster loses nothing with the humans, but does
   lose reputation with the monster factions.
3. **XP rule (against abuse):** resistance XP from hits by players is only noted down. It
   is credited only when the fight ends with a defeat – no matter on which side. So someone
   always pays the respawn penalty. Hits by monsters and the environment still count
   directly.

**Later (at the earliest late stage 2, more likely stage 3): wars.** Cities or regions can
declare war on each other. Then the reputation penalties for attacking your own species
no longer apply, or are even reversed. This needs many players.

## Cities, relations and zones

### Three relations

| Relation | Attacking | NPC behavior |
|---|---|---|
| **friendly** | not possible at all (group, guild, alliance, friendly NPCs) | – |
| **neutral** | only after a deliberate decision (a switch or explicit targeting) | attack only when provoked |
| **hostile** | always possible | attack as soon as they **perceive** you – stealth against perception decides whether you get through unnoticed |

### How the relation is set (in this order)

1. **Hard reasons make hostile** – reputation does not change this:
   - untransformed monsters towards humanoids (hostile at once);
   - monsters of species other than your own (attack at once or when provoked) – so
     monsters have far fewer places to retreat to;
   - outlaws and bandits;
   - a bounty, towards the guards;
   - a hostile organization or an active war.
2. **Group, guild and alliance make friendly** (matches the friendly fire setting).
3. **Otherwise the reputation level decides** – per city or faction and species group:

   | Reputation | Relation | Access |
   |---|---|---|
   | hostile | hostile | guards attack |
   | distrustful | neutral | outer districts or market only, higher prices, no quests |
   | neutral | neutral | trade, inn |
   | friendly | friendly | quests, special merchants |
   | allied | friendly | discounts, own buildings or citizenship (bridge to the [strategy layer](strategy.md)) |

   Reputation is gained through quests, trade, fending off threats and so on.

### Attacking neutrals – consequences depend on the target

- Whoever deliberately attacks a neutral target loses reputation or gets a bounty (as with
  offenses, see [Friendly fire and reputation](#friendly-fire-and-reputation)) – depending
  on the target:
  - **Wild monsters** are not a faction of their own as a species – no reputation loss.
  - **Monster players** can belong to a faction – then reputation loss there.
  - **Bandits and outlaws** bring no negative reputation with their race, if anything
    positive. They are usually hostile rather than neutral anyway.

### Shifters and Demons

- **Shifters:** a humanoid race of its own for transformed monsters (see humanoid form for
  monsters in [monsters.md](monsters.md)). This way a transformed monster can also enter
  cities it would never have been allowed into in monster form.
- **Demons:** a humanoid race that is generally friendlier towards monsters. They run
  **outposts and trading posts** that untransformed monsters can enter too, and that count
  as **safe zones** – the important places of retreat for monsters. Demons are also
  **playable** (unlockable), otherwise like other humanoids (cap 100).

### Travelling merchants

- **Appear at random** in the world – especially far from Demon outposts – and move on
  after a while. They let monsters in particular trade far away from any place of retreat.
- **Completely neutral** towards all species, races and factions.
- **A small, absolute no-attack zone** around them.
- Goods: everyday goods and a small, changing selection of rare items.
- **Against abuse** (luring enemies to the merchant and escaping into the zone): whoever
  enters the zone loses the attention of NPC enemies (they retreat). Whoever is in the
  middle of a fight cannot trade for a short time. The zone stays small.

### Zones and towers

- **Safe zones:** you cannot attack yourself (except in active wars or similar). But if the
  zone belongs to a faction that is hostile to you, its towers, NPCs and players can attack
  you – entering hostile territory is a risk.
- **Defense towers** (inspired by MOBAs) watch important spots and attack all hostile
  players at once – with very high damage that cannot be dodged.
  - **Deliberate exception to the guiding principle** ("you can react to everything"):
    towers are not a combat opponent; they seal off an area. Their range is visible, and
    you get a **warning shortly before entering** the range of a tower that sees you as
    hostile. The reaction is: don't go in.
- **Phase 3:** wars with NPC units that march against towers as in a MOBA, to take them
  over. Rulers decide how their guards see other races and species (see
  [strategy.md](strategy.md)).

## Monetization (first thoughts, still to be analyzed in depth)

- Model **Guild Wars 2:** buy once, with a free, limited version; expansions (DLCs); a
  real-money currency only for cosmetics and comfort (QoL); real-money currency and in-game
  gold can be exchanged for each other.
- To check in the analysis: through the exchange, gold can be bought. If gold buys power
  (rare raw materials, masterworks, unlock currency), it indirectly becomes pay-to-win.
  Related: can the unlock currency be bought, or only earned?
- Mods: cosmetic mods on private servers compete with a cosmetics shop – acceptable there.
  But paid DLC content must not be unlockable through a mod.

## Look

- Stylized, rather drawn; no high-end graphics needed.
- **Whether it becomes a toon look is still open.** The decision depends on the results of
  the tech test (check item "look"; see [tech stack](../tech/tech-stack.md)).

## Expansion in three stages

Each stage builds on the previous one.

1. **Combat arena:** a small arena, focused on characters and the combat system. Tests the
   core (combat, classes, evolution) with little content; online against each other or
   against waves.
2. **Fixed map(s) with dedicated servers:** target per server about **100 players** (200
   would be optimal, but not a must) plus many NPCs. Sandbox world, survival, factions;
   servers hosted by players or officially.
3. **MMO:** a persistent, large world. Only tackled once 1 and 2 hold up.

Important: build server-authoritative already in stage 1, so the network layer can grow
with the game up to stage 3. (The stages map to the phases in the [roadmap](../roadmap.md).)

## Mods

- **Mod support for private dedicated servers**, at least on PC – in hydra Phase 1 and 2
  (arena and fixed maps). Official servers and the MMO stay without mods (fairness,
  protection against cheats; at most pure UI mods later). Consoles without mods.
- **Content as data instead of code:** forms, body skills, elements, debuffs, weapons,
  materials, recipes and schematics are data files with stable IDs and a version number.
  The building-block design (form × element, component × material, spell form × element)
  fits this very well – a new line or a new material is mainly a new file. As a side
  effect it helps agents, because text is easy to read.
- **Two kinds of mods:**
  - **Content mods** (new forms, weapons, maps, models): the client needs them too –
    automatic download when joining the server.
  - **Server mods** (rules, numbers, events): run only on the server; cleanly possible
    thanks to the server-authoritative architecture.
- **Security:** mod logic through an embedded, sandboxed scripting language (e.g. Lua)
  instead of free code; plus data files and models.
- **Server settings:** values such as threat rating, costs and time windows as data, so
  operators of private servers can adjust them even without a mod.
- **Legal:** mod guidelines (who owns mods, what is allowed); possibly a platform such as
  mod.io or Steam Workshop.

## Dungeons

- **Floors going down:** each floor is harder than the one before; solo or as a group.
  **A checkpoint on every floor**, so you don't have to start from the top.
- **Mixed layout:** most floors are put together at random from rooms and corridors. Boss
  rooms are built by hand.
- **A floor boss on every floor:** only once it is defeated do the **loot chest** (items or
  materials and money; rarity rises with depth) and the way to the next floor open.
- **A champion boss every few floors** (every 5 or 10): it is the only enemy on that floor
  and especially strong – a highlight.
- **Dungeon monsters drop only cores**, no materials like monsters on the surface – they
  were "created" by the dungeon. Cores are the catalyst for raw materials (see crafting in
  [items.md](items.md)). So in a dungeon, materials come only from the chests.
- **Difficulty:** rises with depth, and also with the group's threat rating.
- **Enemies** follow the same rules as players. Bosses have telegraphed area attacks and
  phases. Floors or dungeons can have an element as a theme (element fields, debuffs,
  matching material in the chests).
- **Partly already in the arena:** a dungeon mode of a few rooms with waves and a boss. It
  tests early what PvE needs: AI enemies under the same rules as players, group combat,
  reviving (helping up), hXP split as in group fights (see [progression.md](progression.md)), telegraphed area attacks.
- **Stage 2:** fixed dungeons in the world with randomly assembled floors.
- **Stage 3:** large dungeons for groups; events for mythic items.
