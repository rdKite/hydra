# Dev log

Newest entries at the top. Per session: date, start and end time, what was done per
roadmap item, **interventions** (manual editor steps by a human), assumptions, problems and
workarounds.

## Interventions

| Date | What did a human have to do? | Why couldn't it be done without? |
|---|---|---|

## Sessions

### 2026-11-16 – M1 Foundation (agent session, start 14:40)

**Environment check.** Engine 5.8.4 (launcher build) at `D:\dev\Epic Games\UE_5.8`,
`Build.bat` and `UnrealEditor-Cmd.exe` present, Visual Studio 2026 (MSVC 14.52, Windows SDK
10.0.26100) builds a throwaway C++ project, Git LFS 3.8.1 installed.
**`UE_ROOT` was not set** (not in the process, user or machine environment). The engine sits
at the path `CLAUDE.md` gives as the example, so the session passed `UE_ROOT` per command and
went on; the system environment was not changed. The scripts stop with a clear message if
`UE_ROOT` is missing (`Scripts/env.sh`).

**Item 1 – project from text files.** `Hydra.uproject`, `Config/Default*.ini`, two targets,
module `Source/Hydra/` written by hand, no editor wizard. Plugins: GameplayAbilities,
EnhancedInput, PythonScriptPlugin (editor only). Build: `Scripts/build.sh`.

- Problem: the editor adds an `AndroidFileServer` block with a random token to
  `DefaultEngine.ini` on every first start. Workaround: plugin disabled in the `.uproject`.

**Item 2 – values subsystem.** `UHydraValuesSubsystem` (game instance subsystem) loads
`data/values.json` into `FHydraValues` (snake_case members, mapped 1:1 by
`FJsonObjectConverter`); console command `Hydra.ReloadValues`. Lookup: `-HydraDataDir=`,
then `data/`, then `Config/Hydra/` (build copy for packaged builds). Tests
`Hydra.Values.*`: load, every key mapped, every struct member present in the file, reload
through the subsystem (edited value is live, broken file keeps the old values).
Verified: `Scripts/run_tests.sh` passes; with a stray key added to `values.json` the run
fails with `key not read by the game: stamina.typo_key` (exit code 1).

- Beyond the roadmap wording: the reverse check (struct member without a key) – without it a
  removed key would silently run as 0. Stray and missing keys are also logged as warnings
  at runtime on every (re)load.

**Item 3 – data files with stable IDs.** `data/elements.json` (fire), `data/skills.json`
(sword_fighting, shield_fighting, dodge, fire_magic), `data/weapons.json` (longsword,
round_shield), `data/forms.json` (humanoid, slime); structs in `HydraDefinitions.h`, loaded
and reloaded together with the values. Validation at load and in tests: IDs are lower
snake_case and unique per file, references between files resolve.

- **Assumption:** the definition files hold identity and relations only (names, line, family,
  damage types, loadout, skills). **All numbers stay in `values.json`** for now, as
  `phase-1.md` says. Where per-form and per-weapon numbers go (weight, strength, level cap)
  is open – see "To decide before M2" at the end of this entry.
- **Assumption:** skills got first entries too (the four skills Phase 1 names), because
  weapons and elements refer to them.
- **Assumption:** ID format is plain lower snake_case without a namespace prefix. A prefix
  for mods (`hydra:slime`) can be added when mods come (Phase 2).

**Item 4 – GAS helper layer.** `Source/Hydra/Abilities/`: `UHydraGameplayEffect` (base class;
an effect is a subclass whose constructor calls `MakeInstant/MakeTimed/MakeInfinite`,
`AddAmountTo`, `GrantTag`, `PauseWhileTargetHas`, `StackPerTarget`), native tags in
`HydraTags`, `HydraGas::Apply` (builds context and spec, sets the SetByCaller magnitudes
`Data.Amount` and `Data.Duration`), `HYDRA_ATTRIBUTE_ACCESSORS`. First users: the three
stamina effects (change, regeneration, regeneration delay) and `UHydraAttributeSet`.
Verified: test `Hydra.Stamina.CostRegenDelay` runs the real effects on a real ability system
component in a throwaway world with the numbers from `values.json` (cost, no regeneration
during the delay, a second action restarts the delay, rate, clamp at 0 and max, new numbers
after a reload).

- Taken over from the tech test: effects as native classes, GE components as default
  subobjects, the deprecated `StackingType` member (the setter is not exported in 5.8).
- Regeneration runs in fixed 0.1 s ticks (`UHydraGE_StaminaRegen::TickS`). Treated as a
  technical constant, not a gameplay value: the rate per second comes from `values.json`.
- No ability base class yet – M1 has no abilities; it comes with the first attack in M2.

**Item 5 – host mode and network emulation.** `AHydraGameMode` spawns players at the arena's
spawn points; a second window joins with `127.0.0.1` (`Scripts/play.sh`).
`AHydraGameState` replicates the latency step and the packet-loss switch; every process
applies them to its own net driver (engine packet simulation): outgoing lag = step ÷ 2
(steps are round trip), loss = `packet_loss_ratio` per direction. Console
`Hydra.LatencyStep [ms]`, `Hydra.PacketLoss [0|1]`, keys F6 / F8, from host or client.
Verified in the live run: both sides log the applied setting; measured ping 12 ms without
emulation, about 120 ms at the 100 ms step (frame time on both ends is included); the client
switched packet loss off through the server.

- **Problem:** with two game windows on one machine the window in the background dropped to
  8 fps (the host, as soon as the client window had focus). Not a network effect: both
  windows rendered uncapped (~250 fps) and Windows gives the foreground window the GPU. The
  low host frame rate also inflated the ping (83 ms without emulation). **Workaround:** frame
  cap of 120 fps, `[Hydra] MaxFPS` in `Config/DefaultGame.ini`, applied in
  `AHydraPlayerController::BeginPlay`. A plain `t.MaxFPS` in an ini does not hold: the
  engine's game user settings overwrite it at start with their own `FrameRateLimit`.
- Packet loss switches on with the first latency step above 0 and off at step 0 (as in the
  tech test); `packet_loss_both_directions: false` makes only the server drop packets.
- Network emulation needs `DO_ENABLE_NET_TEST`, so it is not in shipping builds.

**Item 6 – arena.** `AHydraArena`: 40 m square, border wall, three pillars, two low covers, a
block and a wall segment, own lighting, all from engine basic shapes in C++. The map
`Content/Maps/Arena.umap` holds just this actor and is written headlessly by
`Scripts/create_arena_map.sh` (Python commandlet). Verified: screenshots of the live run.

**Item 7 – camera, soft aim assist, lock-on.** Third-person camera over the right shoulder
(spring arm on `AHydraCharacter`). Enhanced Input actions and the mapping context are built
in C++ (`AHydraPlayerController`, binding table in the header), mouse/keyboard and gamepad.
Lock-on (Q, middle mouse, right stick click) picks the best character in the cone around the
camera direction (`lock_on.*`), then the controller turns **only the camera** to the target;
the character follows the camera direction through the movement component at its turn rate.
The lock state travels in CharacterMovement's saved move (`FLAG_Custom_1`, as in the tech
test), so the server uses the same facing. Verified in the live run: both players lock on,
camera on target, server has the client's facing; then, with the turn rate lowered to 60°/s
by a reload, the camera is on the target after 0.5 s while the character has turned only
32° and catches up later. Pure rules (cone, score, aim-assist yaw, turn-rate floor and cap)
are unit-tested (`Hydra.Camera.*`).

- **Assumption (aim assist):** as in the tech test, aim assist is a one-off partial turn
  towards the best target in the assist cone (`aim_assist.angle_deg`, `range_m`) by
  `strength`, meant for the start of an attack. M1 has no attack, so it is implemented and
  tested (`AHydraCharacter::ApplyAimAssist`) and can be triggered with the console command
  `HydraAimAssist`; M2 calls it from the attack. This turn is instant, i.e. **not limited by
  the turn rate** – to decide, see below.
- **Assumption (turn rate):** M1 uses `turn_rate.base_deg_s` inside floor and cap for every
  character. Weight ÷ strength and the slowdown while blocking/attacking are M2.
- **Beyond the roadmap wording:** one passive **training dummy** (a character without a
  controller) stands in the arena, so lock-on and aim assist can be tried in a single window.
  It is not the AI opponent of M2.
- The character is a placeholder from engine primitives (cylinder, sphere, a dark visor that
  shows the facing; own character blue, other players red, dummy sand). No Mixamo assets were
  taken over; models come with the Blender pipeline in M3.
- Camera numbers (arm length, shoulder offset, lag, lock-on follow speed, gamepad look
  speed) are in code as camera feel, not in `values.json`.

**Item 8 – stamina.** Maximum, regeneration and regeneration delay from `values.json`
through the effects of item 4, on every character; `HydraGas::CanAfford` /
`HydraGas::SpendStamina` are what actions will call from M2 on. HUD: own bar (its length
grows with the maximum) and bars over other characters. Verified: automation test (item 4)
and live run – cost on the server, replicated to the owner and to the other player, no
regeneration during the delay, regeneration, new maximum after a reload on both sides.

- M1 has no action that costs stamina. For testing there is the console command
  `HydraSpendStamina <amount>` (server RPC, compiled out of shipping builds).
- "Blocking stops regeneration" is prepared (`State.StaminaRegenBlocked`) and used in M2.

**Item 9 – scripts.** `Scripts/run_tests.sh` (build + 11 automation tests, headless),
`Scripts/run_live.sh` (host + client window, automated input through
`UHydraDevSubsystem`: `-HydraExec`, `-HydraShot`, `-HydraQuit`, `-HydraWaitPlayers`; 68
checks from the logs in `Scripts/check_live.py`, four screenshots),
`Scripts/play.sh` (play by hand). Result of the last run: tests 11/11, live run 68/68.

- The dev clocks of host and client start when both players are in the game, so the script
  times of the two windows line up to about 0.1–0.2 s.
- The live run reads a copy of `data/` (`-HydraDataDir=Saved/Live/data`) and edits that copy
  for the reload check, so the real data files are never touched.
- `check_live.py` runs with the Python that ships with the engine (no extra install).
- Problem: Git Bash rewrites arguments such as `/Game/Maps/Arena` into Windows paths.
  Workaround in the scripts: `MSYS_NO_PATHCONV=1`.
- Problem: a first attempt to write several files with one long shell command failed
  (command line too long on Windows); files are written one by one.
- Final check on a **fresh clone** of `main` (includes the LFS map): build, tests 11/11 and
  live run 68/68. It found one bug – `Scripts/build.sh` failed without an existing `Saved/`
  folder – fixed.

**Not done / left open in M1:** nothing from the roadmap list. No packaged build was made
(not asked for in M1; `Hydra.Build.cs` already stages the data copy for it).

**Interventions:** none. No manual editor step was needed; the editor UI was never opened.

**Ideas noted, not built:** per-form data (speeds, stamina) instead of one global set – M2
needs it for weight and strength; stats overlay key for playtests; a `Scripts/setup.sh` that
checks the environment for a new machine.

**To decide before M2 (human):**

1. **Where do per-form and per-weapon numbers live?** (weight, strength, level cap, stamina
   and mana per form, reach and damage per weapon). Options: sections in `values.json` keyed
   by ID (`"forms": {"slime": {...}}`), or numbers inside `forms.json` / `weapons.json`. The
   first keeps "all numbers in one file" as `CLAUDE.md` says; the second keeps a form in one
   place. M2 starts with weight and strength, so this is needed first.
2. **Should aim assist respect the turn rate?** The tech-test version snaps the character by
   up to `strength × angle` at attack start, which is a turn faster than the turn rate – in
   tension with "turn rate always applies". Alternative: aim assist only biases the direction
   the character turns towards, at its turn rate.
3. **`UE_ROOT` is not set on this machine.** Set it once
   (`setx UE_ROOT "D:\dev\Epic Games\UE_5.8"`), otherwise every script stops with a message.
4. **Frame cap 120 fps** (`Config/DefaultGame.ini`) – fine as a default?
5. **Keep the training dummy** until the M2 AI opponent replaces it?

Session end: 15:45.

### 2026-11-16 – repository set up

- Repository skeleton: agent instructions, roadmap, glossary, Phase 1 design, starting
  values (`data/values.json` v1, taken over from the tech test and adapted to positional
  combat), tech-test findings, Blender pipeline notes. No Unreal project yet – that is the
  first item of M1 and needs a machine with the engine.
