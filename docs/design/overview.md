# hydra – fantasy survival RPG: rise as a hero or as a monster

This is the entry point to the full hydra design. It says what the game is, why it is
worth making, which principles hold across all parts, and how hydra compares to existing
games. The details live in the topic files below. Terms follow `GLOSSARY.md`.

## Design files

| File | Content |
|---|---|
| [overview.md](overview.md) | This file: the game in one sentence, why, the two sides, guiding principles, competition |
| [progression.md](progression.md) | Levels, skills, attributes, nXP and hXP, affinity and imprint, group fights |
| [combat.md](combat.md) | Positional combat: attacks, directional block, dodge, turn rate, weapons, stealth and sight cover |
| [magic.md](magic.md) | Stamina, mana and reserve; spells, augments, elements, element fields and debuffs |
| [monsters.md](monsters.md) | Monster lines and forms, evolution, body skills, humanoid form for monsters |
| [items.md](items.md) | Armor, crafting (components, materials, schematics) and rarity |
| [world.md](world.md) | Friendly fire, reputation, cities and zones, dungeons, mods, monetization, look |
| [strategy.md](strategy.md) | The strategy layer of Phase 3 (MMO): territories, rule, control, war |
| [open-questions.md](open-questions.md) | Questions that are still open |
| [phase-1.md](phase-1.md) | The rules for the current prototype (Phase 1 arena) |
| [later-phases.md](later-phases.md) | Checklists for Phases 1b, 2 and 3; the finished tech test |

Related: [tech stack](../tech/tech-stack.md), [roadmap](../roadmap.md),
[decisions](../decisions.md).

## In one sentence

An action survival RPG in an open fantasy sandbox. You either level up classically as a
humanoid (human, elf, …) – or you start small as a monster and evolve step by step.

## Why

The monster rise fantasy from "That Time I Got Reincarnated as a Slime" and "So I'm a
Spider, So What?" hardly exists as a game: start as a weak creature, gain abilities,
evolve into branching forms. Together with a classic RPG side, this gives two very
different ways to play in the same world.

## Two sides

- **Humanoid:** classic RPG – levels, attributes, skills, equipment, crafting.
- **Monster:** evolution instead of equipment. Monsters gain abilities through fighting and
  training, and through body skills (see [monsters.md](monsters.md)). Eating/absorbing is
  put on hold for now. Evolution trees branch depending on play style (whoever uses a lot
  of poison can evolve into a poison form). Body and moveset change with every step.
- Both sides meet in the world – conflict, but possibly alliances too.
- **Monsters are weaker at first** than humanoids and harder to play, but have much higher
  growth potential. Whoever sticks with it ends up stronger.

## Playing hard pays off

- Whoever makes the game harder for themselves is rewarded in the long run. Fights against
  stronger enemies or under harsh (environmental) conditions – cold, darkness, poison, no
  healing potions – give much more skill XP relative to character XP.
- Result: two characters at the same level can be very different in strength. Whoever
  often seeks hard fights instead of easy ones has much stronger skills at the same level.

## Fairness for all play styles

"Playing hard pays off" must not mean "playing easy is pointless".

### Basic promise – everyone reaches this

- **Rare forms** (the typical final form, cap 100). They depend on imprints and stats, not
  on the hardship rating.
- **Humanoids up to level 100.**
- **Equipment up to rarity "rare"** from normal schematics.
- **All content of the world** (dungeons, territories, cities) – only at different depths
  or difficulties.
- These explicitly stay rewards for playing hard: epic, legendary and mythic things, and
  hidden classes (hardship rating, or in Phase 3 its own system).

### Raising skills further at max level

- Through **veteran levels** (see [progression.md](progression.md)): after max level, each
  level, earned through nXP, gives one skill level, up to 150. Whoever plays easy catches
  up this way over time. Whoever plays hard has their skills at the top earlier – a lead in
  time, not a permanent one.
- The XP needed rises linearly – a soft cap: nobody brings all skills to the maximum with
  reasonable effort.
- This replaces the earlier route via items from repeatable achievements.

### Opponents that match your strength

- **Two separate kinds of matchmaking** (arena, PvP): "normal" uses the **threat rating**
  (strength of the character) and "ranked" uses a **rank** or Elo value (skill of the
  person, from wins and losses). Rank from Phase 1b.
- Dungeons scale by depth and threat rating. PvP zones and friendly fire rules let players
  avoid PvP (see [world.md](world.md)).

### Other ways to be valuable

- Crafting grows by use, perception through exploring and detecting – neither through hXP. Whoever prefers making
  things can become a sought-after smith.
- In groups, everyone gets the full nXP and the same share of the hXP.
- Players choose harsh conditions themselves. The world hints at what playing hard leads
  to – a deliberate choice, not a must.

## Sandbox

- No linear story. The world, factions and players create the stories.
- Survival elements (resources, camp/nest, building) – rough and harsh. At your own camp
  or nest, building is free (with limits). The strategy layer instead has fixed building
  sites with states (see [strategy.md](strategy.md)).

## Competition (research 2026-11-13)

**Result:** No game found combines action combat, evolution through branching forms, a
humanoid side and a multiplayer sandbox. The gap is there.

| Game | What it is | What is missing, or what we learn |
|---|---|---|
| **Tensura: ISEKAI Chronicles** (licensed game) | 2D side-view action, combat with combos, town building | mixed to weak reviews: combat praised, but monotonous dungeons, little depth in building and progression; mainly for fans |
| **Spider: Ruler of the Labyrinth** (licensed game) | browser dungeon RPG with an evolution tree, free-to-play | single player, no action combat |
| **Path of Titans** | dinosaur survival MMO, grow and fight, up to 200 players, PvP, mild death penalty | growth instead of branching evolution, no humanoid side – but shows: there is an audience for "grow as a creature, online, PvP" |
| **The Isle** | hardcore dinosaur survival, permanent death, one character slot | in early access for about ten years – a warning against too large a scope |
| **Evolve** (2015) | 4 hunters against 1 monster that grows stronger by eating | asymmetric, but matches only; servers shut down in 2018 |
| **Spore** | evolution starting from the cell | single player, no serious combat |
| Smaller titles (e.g. *Reincarnated As A Monster*, *Summoner Slime*, *EvoCreo*) | monsters with evolution, often mobile, turn-based or roguelike | single player, no action combat |

### What this means for hydra

- **The combination is the unique selling point:** monster evolution with depth, tactical
  positional combat with heavy hits, a humanoid side in the same world, online.
- **Depth of evolution is what fans miss:** the licensed games are criticised mainly for
  shallow progression. That is exactly where hydra comes in, with forms, imprints and
  instincts.
- **Accessible, not hardcore:** Path of Titans is seen as more beginner-friendly than The
  Isle and succeeds with that. hydra's mild respawn penalty (loss of collected skill XP, no
  permanent death) is on this side.
- **Keep the scope under control:** The Isle shows how long a survival project that is too
  large can stay in early access – one more reason for the small arena prototype first.
- **Asymmetry monster vs. humanoid** was already tried by Evolve, as a match game. hydra
  moves it into a persistent world with factions.

Sources: [Wikipedia: Evolve](https://en.wikipedia.org/wiki/Evolve_(video_game)),
[RPGFan: Tensura ISEKAI Chronicles](https://www.rpgfan.com/review/that-time-i-got-reincarnated-as-a-slime-isekai-chronicles/),
[Game8: Tensura ISEKAI Chronicles](https://game8.co/articles/reviews/3978),
[G123: Spider – Ruler of the Labyrinth](https://g123.jp/game/kumo?lang=en),
[Mein-MMO: Path of Titans](https://mein-mmo.de/en/new-survival-mmo-turns-you-into-a-dinosaur-lets-you-fight-with-200-players-for-food-and-partners,857106),
[XGamingServer: The Isle vs. Path of Titans](https://xgamingserver.com/blog/?p=22130),
[Steam: Reincarnated As A Monster](https://store.steampowered.com/app/1201000/Reincarnated_As_A_Monster/),
[Wikipedia: EvoCreo](https://en.wikipedia.org/wiki/EvoCreo).
