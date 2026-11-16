# Tech stack

## Decision (2026-11-16): Unreal Engine 5 – for hydra and the sister project hearth

Chosen with a tech test (Unreal vs. Unity, same small networked melee scope, built by agents
in parallel; details in [`tech-test-findings.md`](tech-test-findings.md) and the repository
`rdKite/hydra-tech-test`).

**Test result**
- Both engines: 12/12 must-have items, one intervention each (approving the MCP check),
  about the same time (~2.5 hours).
- **Agents work well with Unreal:** logic in C++, assets by headless Python commandlets,
  values as JSON – zero manual editor steps in the must-have scope. The feared downside of
  binary assets did not show in the test.
- **Networking built in:** predicted movement via CharacterMovement (no corrections at
  100 ms and 2 % loss), GAS with prediction; Iris with a 20 m interest range cut bandwidth by
  65–75 %.
- **Performance:** host mode with 200 characters ~70 fps (Unity 22 fps, partly because of
  its shader); server tick ~10 ms at 200 characters (budget 33 ms).
- **Fits the design:** GAS for stamina, mana, reserve, stacking debuffs and augments; turn
  rate is a built-in CharacterMovement value (`RotationRate`).
- For Unity: everything as text, better MCP integration, easier mods (Lua), headless server
  builds without a source engine. That weighs less than networking and scaling.

**Rules for Unreal** (see also [`../../CLAUDE.md`](../../CLAUDE.md))
- Logic in C++, Blueprints at most as thin asset assignment; animation graph in C++ where
  possible.
- Assets (maps, FBX import, materials) by Python commandlet, repeatable and without editor.
- Values as JSON, reloadable at runtime; build and automation tests from the command line.
- A small own helper layer over GAS against the C++ boilerplate.

**Open risks**
- **Dedicated server** needs a source-built engine (long builds, much disk; 64 GB RAM
  recommended). Phase 1 in host mode works with the launcher engine – needed from Phase 1b.
- The test avoided Animation Blueprints, Niagara and monster rigs.
- **Lag compensation** for melee hits and block arcs is not built in – build it ourselves.
- **Mods** are heavy: plan data mods early, check Lua via a plugin, content mods (pak or
  plugins) later.
- Only one client was measured; many clients are untested.
- License: free up to USD 1 million revenue, then a revenue share (checked 2026-11-16) –
  include it in the business plan.

**Candidates before the decision**

| Option | For | Against |
|---|---|---|
| Unreal Engine 5 | dedicated server and replication built in, proven up to near-MMO scale, strong console support | heavy and large, C++/Blueprints, a stylized look needs own shaders |
| Unity (C#) | many cozy games, good console ports, Netcode/Mirror/Fish-Net | licensing policy was unsettled in the past |
| Godot 4 | open, light, quick start | consoles only via porting partners, networking for large worlds mostly self-built |

- **Godot excluded** (2026-11-13): networking for action combat would mostly be self-built –
  exactly the riskiest part.
- **Built mainly by agentic engineers**, not only classic developers; learning C++ is no
  obstacle. Important for the choice: logic as text code that agents can read and change.

## Further notes

- **Language:** repositories for hydra and hearth are in **English** (code, docs, agent
  instructions). [`GLOSSARY.md`](../../GLOSSARY.md) maps every design term to one English
  name (e.g. no "blueprint" for crafting plans, because of Unreal).
- **3D pipeline:** Blender as the source for models, rigs and animations, made by scripts –
  see [`blender-pipeline.md`](blender-pipeline.md).
- **MMO stage (Phase 3):** additionally an own server layer (e.g. Rust or C#) or solutions
  like SpacetimeDB – to check later. One server per world won't be enough in any engine;
  zones, sharding or several servers per world are a project of their own.

## Shared with hearth

hearth is a planned cozy survival game by the same developer, using the same stack:
- Same engine, network layer and graphics pipeline; character, inventory, crafting and
  building systems could be shared.
- Possibly the same universe.
- The skill system "better by use" (active and passive skills) as a shared core; hydra adds
  character levels and the hardship factor.
- Stamina and reserve as a shared mechanic; hearth with much milder penalties (only slower
  regeneration).
- Order: the arena (Phase 1) is small enough to prove the shared stack before either of the
  two big games is built.
