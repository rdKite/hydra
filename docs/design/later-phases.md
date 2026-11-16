# Later phases (1b, 2, 3)

Phase 1 (arena prototype, roadmap milestones M1–M5) lives in [../roadmap.md](../roadmap.md)
and [phase-1.md](phase-1.md). This file holds the task checklists for the phases after it.
They are plans, not commitments: build only what the current milestone asks for. Terms
follow `GLOSSARY.md`; decisions behind these items are in [../decisions.md](../decisions.md).

## Phase 1b – complete the arena (stage 1 complete)

- [ ] **Online:** play over real servers, PvP humanoid against monster.
- [ ] Numbers and rules as server settings, prepared for private servers.
- [ ] More weapons: spear, short bow; free choice of hands, switching grip.
- [ ] Ice as the opposite element; opposites cancel each other out; more spell forms.
- [ ] Hand wheel per hand and magic wheel; augments with bound mana.
- [ ] Starting forms of wolf and saurian with first evolutions.
- [ ] Threat rating and hXP; groups, reviving, hXP split evenly within the group; overview
      after the fight.
- [ ] **Dungeon mode:** a few rooms, waves, one boss with telegraphed area attacks.
- [ ] Fixed equipment sets; matchmaking "normal" (threat rating) and "ranked" (rank).
- [ ] First balancing numbers from the measurements (stamina, mana, block arcs, turn rate, chains,
      augments, debuffs).
- [ ] Alpha with all species open.

## Phase 2 – fixed maps on dedicated servers

- [ ] World, survival and camp or nest; reserve, resting, inn.
- [ ] Exploring; sneaking and perception, sight cover from bushes, tall grass and crops (AI
      sight check by raycast); resistances from the environment.
- [ ] Crafting from components, materials and recipes, rarity up to legendary, magic bag,
      weapons worn on the body.
- [ ] Armor from material and schematic.
- [ ] Full level system with all forms of the first lines; respawn penalty.
- [ ] Veteran levels after max level (nXP, rising linearly, one skill level each, up to
      150); skill titles every 30 levels, traits in between.
- [ ] Travelling merchants with a no-attack zone; starting alignment (an element or
      elementless) with a matching environment in the tutorial or a matching starting point
      for monsters; awakening of an element through the first imprint level.
- [ ] Factions, reputation, friendly fire; three relations, reputation levels, Shifters and
      Demons (trading posts as safe zones), defense towers with a warning.
- [ ] Dungeons in the world (floors, loot chests, cores).
- [ ] Building materials: regional look of settlements from local materials; a small base
      set per material tier (T1–T3) for walls and roofs, also for building at the camp
      (see [strategy.md](strategy.md#building-materials-regional-look)).
- [ ] Unlocking species with in-game currency.
- [ ] **Mod support** for private dedicated servers (PC): content and server mods,
      automatic download, scripting language with a sandbox, mod guidelines, maybe mod.io or
      Steam Workshop.
- [ ] Decide on monetization (before selling); plan server costs.

## Phase 3 – MMO

- [ ] Persistent, large world; a server solution for large worlds (own server,
      SpacetimeDB …).
- [ ] Changing world: fixed building sites with states and tiers (fields, farms, outposts,
      settlements), changing through population, control, war and time; building tiers need
      a minimum material tier, more stone and wood kinds, import of materials by trade.
- [ ] Strategy layer: territories and rule through seals, a hierarchy in tiers, control and
      patrols, economy with overextension, war with lead time and time windows (NPC units
      against towers), following for leaders; coat of arms and colors of the ruling guild
      (armor, banners, buildings, strategy map).
- [ ] Unique items (e.g. rule over a territory).
- [ ] Later, not at launch: events for mythic items.
- [ ] Unlock epic and legendary forms and legendary gear through achievements and large
      amounts of resources (like the 2nd-generation legendary weapons in Guild Wars 2) –
      based on what Phase 2 teaches.
- [ ] Large dungeons for groups.
- [ ] Account data and GDPR.

## Tech test (done)

- **Goal:** choose between Unreal Engine 5 and Unity by building the same small networked
  melee project in both – a measuring instrument, not a game. Local, host mode, built mainly
  by agents with the same instructions, data file and model; at most five days per engine.
- **Tested:** stamina, server-side hits with hit stop, block (plus the then-current parry),
  a stacking burning effect, JSON values reloadable without recompiling, simulated latency,
  toon shading, metrics, and scaling to 200 replicated AI dummies. Scored: networking,
  abilities, look and performance, agent suitability, MCP, moddability, effort, tokens and
  interventions, scaling, license.
- **Result:** **Unreal Engine 5** was chosen. The parry was only a measuring point for
  server-authoritative hits under ping; hydra has dropped it.
- Repo: [rdKite/hydra-tech-test](https://github.com/rdKite/hydra-tech-test). Findings:
  [../tech/tech-test-findings.md](../tech/tech-test-findings.md).
