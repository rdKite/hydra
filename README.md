# hydra

A fantasy survival RPG: rise as a humanoid hero or as a monster that evolves – from a slime
to a fire elemental, from a wolf pup to something much bigger. Positional, weighty combat;
skills grow by use; online from the start.

**Status:** Phase 1 – arena prototype (local, host mode). Engine: Unreal Engine 5.

## Phases

| Phase | Content |
|---|---|
| **1** | Arena prototype: humanoid and slime against AI waves, one evolution step, one level-up |
| 1b | Arena online (PvP humanoid vs. monster), more weapons, elements and forms, dungeon mode |
| 2 | Fixed maps on dedicated servers: world, survival, crafting, factions, mods |
| 3 | MMO: persistent world, changing landscape, strategy layer |

Details: [`docs/roadmap.md`](docs/roadmap.md).

## Build, test, play

Needs Unreal Engine 5.8 (launcher build), Visual Studio with the C++ game workload and Git
LFS. Set the environment variable `UE_ROOT` to the engine folder once
(`setx UE_ROOT "D:\dev\Epic Games\UE_5.8"`, then open a new terminal). All commands run in
Git Bash from the repository root.

| Command | What it does |
|---|---|
| `Scripts/build.sh` | builds the editor target (used for tests and for playing) |
| `Scripts/run_tests.sh` | build + automation tests, headless |
| `Scripts/create_arena_map.sh` | recreates `Content/Maps/Arena.umap` (only if the map is lost) |
| `Scripts/play.sh` | **play:** host window plus a client window (`host` / `client` for one only) |
| `Scripts/run_live.sh` | scripted live run: host + client, automated input, screenshots, checks |

Without the script, from any shell (after a build):

```
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "C:\code\hydra\Hydra.uproject" "/Game/Maps/Arena?listen" -game -windowed -ResX=1280 -ResY=720
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "C:\code\hydra\Hydra.uproject" 127.0.0.1 -game -windowed -ResX=1280 -ResY=720
```

Controls (one button per hand):

| | Mouse/keyboard | Gamepad |
|---|---|---|
| Move / look | WASD / mouse | left / right stick |
| Weapon hand: press = light attack, hold = heavy attack | left mouse | right shoulder |
| Shield hand: hold = directional block | right mouse | left shoulder |
| Dodge | Space | B / circle |
| Lock-on | Q or middle mouse | right stick click |
| Walk | Left Alt | – |

F5 reloads `data/*.json`, F6 next latency step, F8 packet loss on/off, `~` console:
`Hydra.ReloadValues`, `Hydra.LatencyStep [ms]`, `Hydra.PacketLoss [0|1]`, `Hydra.AI [0|1]`
(AI opponent passive / active, on the host), `Hydra.LagCompensation [0|1]`, `HydraStats`,
`HydraResetAll`, `HydraTeleport x y yaw`. The blade shows the telegraph: yellow in the
wind-up, towards red while a heavy attack charges.

## Where things are

- [`CLAUDE.md`](CLAUDE.md) – rules for the agents that build the game.
- [`docs/design/overview.md`](docs/design/overview.md) – entry point to the full design (all
  topic files).
- [`docs/design/phase-1.md`](docs/design/phase-1.md) – game rules for Phase 1.
- [`docs/decisions.md`](docs/decisions.md) – history of design decisions.
- [`GLOSSARY.md`](GLOSSARY.md) – English names for all game terms.
- [`data/values.json`](data/values.json) – all gameplay numbers.
- [`data/`](data) `forms.json`, `skills.json`, `elements.json`, `weapons.json` – what exists
  in the game, with stable IDs (no numbers; those are in `values.json`, per ID).
- [`Source/Hydra/`](Source/Hydra) – the C++ game module: `Data/` (values and definitions),
  `Abilities/` (GAS helper layer, stamina and health, attack / block / dodge),
  `Character/` (combat rules, hit resolution, lag compensation), `AI/` (practice opponent),
  `Core/` (game mode, network emulation, controller, HUD, dev tools), `Arena/`, `Tests/`.
- [`Scripts/`](Scripts) – build, test, map and run scripts.
- [`docs/tech/`](docs/tech) – tech-test findings, Blender pipeline.
- [`docs/dev-log.md`](docs/dev-log.md) – sessions, interventions, assumptions.

The engine choice was made with a tech test (Unreal vs. Unity) in the separate repository
`rdKite/hydra-tech-test`.
