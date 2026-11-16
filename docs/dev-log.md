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

### 2026-11-16 – repository set up

- Repository skeleton: agent instructions, roadmap, glossary, Phase 1 design, starting
  values (`data/values.json` v1, taken over from the tech test and adapted to positional
  combat), tech-test findings, Blender pipeline notes. No Unreal project yet – that is the
  first item of M1 and needs a machine with the engine.
