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
| `Scripts/play.sh` | **play against waves:** host window plus a client window (`host` / `client` for one only); `HOST_FORM=slime Scripts/play.sh` to play the slime; `WAVES=0 Scripts/play.sh` for one practice opponent instead of waves |
| `Scripts/run_live.sh` | scripted live run, combat: host + client, automated input, screenshots, checks |
| `Scripts/run_live_slime.sh` | scripted live run, slime and magic |
| `Scripts/run_live_waves.sh` | scripted live run, progression and waves |
| `Scripts/run_live_debug.sh` | scripted live run, test tools (debug panel and its commands) |
| `Scripts/playtest.sh humanoid` / `slime` | **playtest:** one window against the waves (add `duo` for a second player window), then the metrics summary |
| `Scripts/shots_humanoid.sh` | the humanoid base model: body presets and short legs in idle, block and attack, one page (`Saved/Screenshots/Humanoid/index.html`) |
| `Scripts/shots_look.sh` | the three look variants side by side: same shots, one page (`Saved/Screenshots/Look/index.html`) |
| `Scripts/summarize_metrics.sh` | summary of the newest metrics CSV: hits, blocked hits, dodges, stamina use per player, enemy type and wave |
| `Scripts/run_all.sh` | tests and the four live runs |
| `Scripts/build_art.sh` | Blender builds models, rigs and textures, Unreal imports them (needs Blender) |

Without the script, from any shell (after a build):

```
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "C:\code\hydra\Hydra.uproject" "/Game/Maps/Arena?listen?Form=humanoid" -game -windowed -ResX=1280 -ResY=720
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "C:\code\hydra\Hydra.uproject" "127.0.0.1?Form=slime" -game -windowed -ResX=1280 -ResY=720
```

Controls (one button per hand):

| | Mouse/keyboard | Gamepad |
|---|---|---|
| Move / look | WASD / mouse | left / right stick |
| Weapon hand: press = light attack, hold = heavy attack | left mouse | right shoulder |
| Off hand – humanoid: hold = directional block; slime: press = bolt, hold = charge it | right mouse | left shoulder |
| Jump | Space | A / cross |
| Dodge | Left Alt | B / circle |
| Lock-on | Q or middle mouse | right stick click |
| Precise movement on / off: the character keeps facing where the camera looks | X | left stick click |
| Walk | Left Ctrl | – |
| At a milestone: evolve / start or raise the fire imprint (only what the HUD offers) | J / K | D-pad up / right |

**F1 opens the debug panel** (a test tool, not in shipping builds): set your form, level,
every skill and the fire imprint, open a milestone, reset your bars, spawn any enemy type
at a level (active or passive), remove all enemies, switch the waves on or off. Arrow keys
or D-pad choose and change, Enter applies, the mouse works too; your character's input is
paused while it is open. Every action is also a console command: `HydraSetForm <form>`,
`HydraSetLevel <n>`, `HydraSetSkill <skill id> <level> [bar 0..1]`, `HydraSetImprint <n>`,
`HydraMilestone`, `HydraReset`, `HydraSpawn <enemy id> <level> [1 = passive]`,
`HydraClearEnemies`, `HydraWaves 0|1`.

F5 reloads `data/*.json`, F6 next latency step, F8 packet loss on/off, `~` console:
`Hydra.ReloadValues`, `Hydra.LatencyStep [ms]`, `Hydra.PacketLoss [0|1]`, `Hydra.AI [0|1]`
(AI opponent passive / active, on the host), `Hydra.LagCompensation [0|1]`, `HydraStats`,
`HydraResetAll`, `HydraRefill` (full stamina and mana), `HydraBody neutral|male|female` and `HydraLegs 0.8`
(looks of the humanoid base model: body preset and leg length), `HydraLookVariant 1|2|3` (look variants: plain, toon, ink), `HydraTeleport x y yaw`, `HydraForm slime|humanoid`, `HydraPrecise 0|1`, `HydraFire 1`
(fire awakened without earning it: the slime's bolt carries fire, leaves a fire field and
causes burning), `HydraWaves 1` (switch a practice session to waves), `HydraXp <n>` and
`HydraImprintProgress <n>` (dev: give yourself nXP or imprint progress), `HydraChoose
evolve|imprint`. The humanoid's blade shows the telegraph: yellow in the wind-up,
towards red while a heavy attack charges. `Hydra.AI 0` makes every AI stand still.

**Waves and progression.** Enemies come in waves (`data/values.json`: `waves`, `enemies`).
Everyone starts at a fixed level (humanoid 9, slime 29 – one level before its cap). A level
raises the body attributes (HP, stamina, mana, handling, damage through strength and
dexterity). Defeated enemies give nXP for the level and hXP for the skills: the hXP go to
the skills you used, by the stamina and mana they spent since the last defeated enemy. At
a level-up you get automatic skill points – more after hard enemies – split by those hXP;
each skill shows a progress bar. Skills raise effect (damage, block arc, fire field), never
lower costs: an attack costs what the weapon weighs against your body. Contact with fire –
ember slimes cast fire bolts, leave fire fields and burn whoever hits them – builds the
fire imprint. At a milestone (every 10 levels, and the level cap) the HUD offers what you
earned: `K` starts or raises the imprint, `J` evolves the slime into the fire elemental
slime (level and skills start again, with an instinct per skill). Every cleared wave writes
a metrics row per player and enemy type to `Saved/Metrics/metrics_<time>.csv`.

## Where things are

- [`CLAUDE.md`](CLAUDE.md) – rules for the agents that build the game.
- [`docs/design/overview.md`](docs/design/overview.md) – entry point to the full design (all
  topic files).
- [`docs/design/phase-1.md`](docs/design/phase-1.md) – game rules for Phase 1.
- [`docs/decisions.md`](docs/decisions.md) – history of design decisions.
- [`GLOSSARY.md`](GLOSSARY.md) – English names for all game terms.
- [`data/values.json`](data/values.json) – all gameplay numbers.
- [`data/`](data) `forms.json`, `skills.json`, `elements.json`, `weapons.json`, `enemies.json` – what exists
  in the game, with stable IDs (no numbers; those are in `values.json`, per ID).
- [`Source/Hydra/`](Source/Hydra) – the C++ game module: `Data/` (values and definitions),
  `Abilities/` (GAS helper layer, stamina and health, attack / block / dodge),
  `Character/` (combat rules, hit resolution, lag compensation, progression), `Combat/`
  (bolt, fire field), `AI/` (practice opponent and wave enemies),
  `Core/` (game mode with the waves, metrics, network emulation, controller, HUD, dev
  tools), `Arena/`, `Tests/`.
- [`Scripts/`](Scripts) – build, test, map, art and run scripts.
- [`Art/`](Art) – Blender scripts and the .blend files they write (models, rigs, textures).
- [`docs/tech/`](docs/tech) – tech-test findings, Blender pipeline.
- [`docs/m5-preparation.md`](docs/m5-preparation.md) – what is ready for the look decision and the playtest: look variants, animation proposal, playtest kit.
- [`docs/dev-log.md`](docs/dev-log.md) – sessions, interventions, assumptions.

The engine choice was made with a tech test (Unreal vs. Unity) in the separate repository
`rdKite/hydra-tech-test`.
