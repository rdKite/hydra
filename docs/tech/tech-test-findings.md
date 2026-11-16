# Findings from the Unreal tech test

The engine was chosen with a tech test (Unreal vs. Unity, same small networked melee scope,
built by agents in parallel). Sources: `rdKite/hydra-tech-test`, `results/log-unreal.md` and
`results/engine-vergleich.html`. The test code is a reference, not a base: it was built
under time pressure and still contains the parry, which hydra has dropped. Take over
patterns and individual pieces on purpose.

## Result in short

- Must-have scope 12/12, one intervention (approving the MCP check), ~2.5 hours.
- Agents work well with Unreal when logic is C++, assets are made by Python commandlets and
  values are JSON – zero manual editor steps in the must-have scope.
- Host mode with 200 animated characters ~70 fps; server tick ~10 ms at 200 characters
  (30 Hz budget 33 ms); ~41 KB/s per client, Iris with a 20 m interest range cut that by
  65–75 %.

## Patterns worth keeping

- **Project from text files.** No "New Project" wizard (editor-only). The C++ Third Person
  template's content packs are only copied by the wizard, so its code was used as reference
  and the content built procedurally.
- **Maps by Python commandlet:** `LevelEditorSubsystem.new_level` works;
  `AssetTools.create_asset(World)` + `load_level` crashed (world still referenced).
- **FBX import headless** by commandlet; the importer strips the `mixamorig:` prefix.
- **Animation without Animation Blueprint:** a C++ `UAnimInstance` with a custom
  `FAnimInstanceProxy::Evaluate` blends locomotion (phase-synced) and one full-body action
  layer. Everything stays text; montages, notifies and the anim editor preview are not used.
- **Mixamo root motion:** clips move the hips, there is no root bone; the anim instance pins
  the hips' X/Y to the reference pose and CharacterMovement moves the capsule.
- **Values:** structs with snake_case members so `FJsonObjectConverter` maps keys 1:1 (saves
  a parser, deviates from UE naming). Automation test `Values.AllKeysMapped` catches keys
  the game would silently ignore. Values changed by reload apply to new actions; running
  effects keep their magnitudes.
- **GAS without assets:** effects as native `UGameplayEffect` subclasses configured in the
  constructor, native tags (`UE_DEFINE_GAMEPLAY_TAG`), magnitudes via SetByCaller.
  - `Period` and `StackLimitCount` belong to the effect class, not the spec → written into
    the class default object on (re)load. Works, but is a CDO mutation.
  - GE components must be created with `CreateDefaultSubobject` in the constructor.
  - `UGameplayEffect::SetStackingType` is not DLL-exported in 5.8 (link error); the
    deprecated `StackingType` member was set directly with warnings suppressed.
  - API drift in 5.8: `AddReplicatedLooseGameplayTag` is gone (parameter of
    `AddLooseGameplayTag`), `AbilityTags` → `SetAssetTags`.
  - An action needs `stamina >= cost` to start; regeneration in fixed 0.1 s ticks.
- **Movement states in the saved move:** switching walk/strafe via RPC causes corrections,
  so states travel in CharacterMovement's saved move (`FLAG_Custom_0/1`, ~150 lines of
  boilerplate). Facing mode derives from the same flag on both sides, so server hit tests
  use the client's facing.
- **Hit detection:** geometric test on the server (reach and arc in front, sampled at 60 Hz
  during the hit window) instead of tracing the blade through the animation – the clip
  timing doesn't match the wind-up, and a dedicated server doesn't evaluate poses.
  **No lag compensation was built** – hydra needs it (rewind positions and facing).
- **Hit stop** was cosmetic only (both clips freeze); the ability timeline was not extended,
  so client and server stay in sync.
- **Hit stun:** human decision in the test – no movement during hit stun (max speed 0 on
  server and owning client); can cause a small correction under latency.
- **Prediction:** the owning client plays its attack clip at once and ignores the server's
  later copy; server-initiated actions (hit react, death) override it.
- **Network emulation:** UE's packet simulation; packet loss is an integer percent.
  Measured ping includes frame time on both ends.
- **Agent-driven checks:** a dev subsystem with `-<Prefix>Shot=<s>`,
  `-<Prefix>Exec=<s>:<cmd>;...` (`|` for spaces) and `-<Prefix>Quit=<s>`; exec commands for
  move, attack, block, dodge, lock-on, stats. Console commands via the player controller
  (`GEngine->Exec` does not reach `HighResShot`). Don't click into automated client windows
  – the window captures the first mouse click.
- **Toon look** as a post-process material created by a Python script (HLSL text inside),
  worked first try; costs a full-screen pass, deferred only. A per-material shading model
  would need engine changes or an editor-made material graph. Look is still open.

## Commands (from the test, adapt names)

All from the project folder in Git Bash, `UE="$UE_ROOT"`.

- Build editor target:
  `"$UE/Engine/Build/BatchFiles/Build.bat" HydraEditor Win64 Development -Project="<abs>/Hydra.uproject" -WaitMutex`
- Python commandlet:
  `"$UE/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" <abs>/Hydra.uproject -run=pythonscript -script=<abs>/Scripts/<script>.py -unattended -nosplash -nullrhi`
- Automation tests (exit code 0 = pass):
  `UnrealEditor-Cmd.exe <abs>/Hydra.uproject "-ExecCmds=Automation RunTests Hydra;Quit" -unattended -nullrhi -nosplash "-testexit=Automation Test Queue Empty" -log=Tests.log`
- Listen server with screenshot:
  `UnrealEditor.exe <abs>/Hydra.uproject "/Game/Maps/Arena?listen" -game -windowed -ResX=1280 -ResY=720 -unattended "-ExecCmds=HighResShot 1280x720"`
- Packaged build:
  `"$UE/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun -project=<abs>/Hydra.uproject -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -iostore -archive -archivedirectory=<abs>/Saved/Packaged -unattended -utf8output`
- Dedicated server target: not buildable with the launcher engine (needs a source build,
  Phase 1b).

## MCP

`runreal/unreal-mcp` over UE's Python remote execution worked without project changes
(inspection, spawn/move, console, Python), but has no build, test or log tools and an
unusable screenshot. Moderate value next to headless commandlets; installing it needs the
human's approval.

## Open risks for hydra

- Dedicated server needs a source-built engine (long builds, much disk and RAM; 32 GB was
  tight in the test).
- The test avoided Animation Blueprints, Niagara and monster rigs.
- Lag compensation for melee hits and block arcs must be built.
- Mods are heavy in Unreal: plan data mods early, check Lua via a plugin, content mods
  (pak/plugins) later.
- Only one client was measured; many clients are untested.
