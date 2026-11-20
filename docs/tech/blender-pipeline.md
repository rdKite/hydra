# Blender pipeline (by script)

Models, rigs and textures are made by agents with Blender Python scripts – no manual
steps, repeatable. **Animations are designed by the human** (in Blender or a tool like
Cascadeur); agents prepare the rigs, export and import them and wire them into the game, and
may make simple placeholder animations by script for tests. Mixamo is only a placeholder.
First used in M3 (slime playable, wolf pup as a pipeline test).

## Rules

- **Blender is the source.** `.blend` files and their scripts live in `Art/` (`.blend` via
  Git LFS). No `.blend` files go into the engine directly.
- **Headless:** `blender -b Art/<source>.blend --python Art/scripts/<script>.py`
  (or without a source file for generated models).
- **Export by script** to **FBX** with fixed settings in one shared export script; import
  into Unreal by Python commandlet (`Scripts/`).
- **One rig per line** (slimes, wolves, saurians). Forms of a line share the rig with other
  proportions – saves a lot of animation work.
- **Materials** don't transfer 1:1: textures are exported, the look is made in Unreal (the
  **plain** look, decision 2026-11-19: standard lighting, no toon pass – so textures and
  materials carry it).

## Textures (procedural, by script)

- Materials are built from Blender shader nodes by Python (noise, gradients, edge wear,
  color variation, vertex colors) and **baked to textures headlessly** (PNG, Git LFS). The
  script is the source; textures are a build product of it.
- Good for stone, wood, earth, slime gel, skin and fur as a base. The more stylized the
  look, the fewer textures are needed – much can come from vertex colors, gradient maps and
  the shader in Unreal.
- Workflow: M3 tries it on slime and wolf pup → M5 decides the look and shows how much
  texture work it needs (decided 2026-11-20: procedural only for now) → only if
  hand-painted detail is needed later, add AI image generation
  (e.g. ComfyUI, workflows as JSON in the repo, local GPU) with style references painted by
  the human. Check model licenses and disclose AI content on Steam if used.

## How to run it (M3)

| Command | What it does |
|---|---|
| `Scripts/build_art.sh` | Blender builds every model (`Art/scripts/build_<name>.py`), then Unreal imports them |
| `Scripts/build_art.sh slime` | one model only |
| `Scripts/build_art.sh import` | only the import (after exporting animations by hand) |

Blender 5.2 is expected under `C:\Program Files\Blender Foundation\`; otherwise set
`BLENDER` to `blender.exe`. Everything runs without a window.

- `Art/scripts/hydra_blender.py` – shared helpers: scene setup, rig, weights, baking,
  export, preview renders.
- `Art/scripts/build_slime.py`, `build_wolf_pup.py`, `build_humanoid.py` – one script per model.
- `Art/<name>/<name>.blend` and `T_<name>_color.png` – written by the build script (LFS).
- `Art/export/<name>/*.fbx` – build products, not in git.
- `Scripts/import_art.py` – Unreal Python commandlet: `Content/Characters/<name>/`
  (`SK_<name>`, skeleton, `T_<name>_color`, `M_<name>`, `A_<name>_<Action>`).
- `Saved/ArtPreview/*.png` – small renders of every build to judge shape and deformation.

The game finds a model by name: a form `<id>` with `/Game/Characters/<id>/SK_<id>` is shown
with it, otherwise with the placeholder body. Animations are looked up as `A_<id>_<Name>`
with the names Idle, Move, Windup, Windup2, Charge, Strike, Strike2, HeavyStrike, Recovery,
Cast, CastCharge, CastRelease, Block, Dodge, Jump, Death. **Windup2 / Strike2** are the second
pair for the light attack: the game alternates the pairs within a chain (first strike the
first pair, the strike after a hit the second, and so on; a new chain starts with the first)
and uses the first pair alone if a model lacks one of the second. Clips of actions are time-scaled to the action's duration from
`values.json`, so their length in Blender does not change the gameplay timing.

## Humanoid base model (M5)

`Art/scripts/build_humanoid.py` builds the one base model for all humanoids. Since
2026-11-20 it is a **realistic human from MakeHuman** (decision of the human, after the
scripted stylised body of 2026-11-19 looked too clunky). Run:
`blender -b --python Art/scripts/build_humanoid.py` (a few minutes with previews;
`-- nopreview` skips them), or `Scripts/build_art.sh humanoid` for build and Unreal import.

**Setup, once per machine:** `Scripts/setup_mpfb.sh` installs the **MPFB2** extension
(MakeHuman for Blender, 2.0.18) and the **MakeHuman system assets** into the user's Blender,
both downloaded with a pinned SHA-256. Licences: MPFB's code is GPL and is only a tool here
(none of it ships); the base mesh, its targets and the assets used (skin, eyes, eyebrows,
eyelashes) are **CC0**. The build stops with a clear message if either is missing.

**Body.** MPFB creates a human from the MakeHuman base mesh with its macro sliders (gender,
age, muscle, weight, proportions, height; values in `MACRO` / `PRESETS`) at centimetre
scale, removes the helper geometry and bakes the targets. About 178 cm (neutral), A-pose.
No clothes and no hair on the body itself: armour comes as equipment, hair as hair styles
(both later). The face proxies are MakeHuman assets: low-poly eyes, eyebrows, eyelashes.

**Shirt and trousers** (separate material slots, so the game can tint and later replace
them). They are cut from the body by script (`garment_plan`, `cut_garment`):
- the body faces they cover (shirt: trunk above the hips and the arms to just above the
  wrist; trousers: waist to just above the ankle), so they keep the body's **UVs and bone
  weights** and deform exactly like the skin under them;
- the hems smoothed along themselves (the cut follows the mesh grid);
- the cloth smoothed strongly (it bridges the spine and the buttocks), then pushed out until
  it is at least its **stand-off** away from a reference skin that is smoothed without
  shrinking (Taubin), so nipples and navel vanish under the cloth but limbs keep their
  size. Stand-off (the cut): shirt 1.7 cm, falling loose to 3.9 cm over the hips, sleeves
  1.4 cm widening to 2.7 cm at the cuff; trousers 1.1 cm at the hips, widening below the
  knee to 2.7 cm at the ankle;
- 3 mm cloth thickness (solidify with rims).

**Material slots and textures** (`Art/humanoid/materials.json`, read by
`Scripts/import_art.py`, which builds `M_humanoid_<slot>`): `body` (MakeHuman skin
`young_lightskinned_male_diffuse`, 2048²), `eyes` (brown), `eyebrows` and `eyelashes`
(alpha-tested, two-sided cards), `shirt` (undyed linen) and `trousers` (grey-brown). The
two cloth textures are made by the script (a calm uneven tone and a fine grain on the
body's UVs). Every material has `Glow`; the **shirt has `Tint`** (the game's side colour),
the trousers have `TrousersTint` (default white, free for equipment colours later).

**Skeleton** (55 bones): MPFB's "game engine" rig – names of the Unreal mannequin, Blender
bone axes (Y along the bone):
- `root` → `pelvis` → `spine_01..03` → `neck_01` → `head`; `spine_03` → `clavicle_l/r` →
  `upperarm` → `lowerarm` → `hand` → `thumb/index/middle/ring/pinky_01..03`
- `pelvis` → `thigh_l/r` → `calf` → `foot` → `ball`
- `weapon_r` (added), child of `hand_r`: in the middle of the closed fist (worked out from
  the fist pose), **+Y of the bone along the blade**, leaving above the thumb.
- `shield_l` (added), child of `lowerarm_l`: on the outside of the forearm, **+Y out of the
  shield's face**.
MPFB's weights; at most 4 bones per vertex. Every bone is a deform bone (the export keeps
deform bones only). The `.blend` holds a stand-in longsword and round shield on the sockets
(not exported).

**Body presets.** Neutral (gender 0.5) is the base; `male` (1.0) and `female` (0.0) are shape
keys on every slot (FBX blend shapes → morph targets). Each preset is a full MPFB build with
the same garment plan, **scaled to the neutral height** so the one skeleton fits; height
differences are the game's job (actor scale).

**Placeholder animations** (`A_humanoid_<Name>.fbx`, 30 fps, 13 clips): Idle (40 frames,
loop), Move (16, loop), Windup (8) / Strike (4) and **Windup2** (8) / **Strike2** (4) –
the light attack, both sides –, Recovery (10), Charge (12, loop), HeavyStrike (4, a
thrust), Block (4), Dodge (14), Jump (12, loop), Death (16). The poses are turns around world
axes per bone (`apply_pose`); finger "curl" turns towards the palm about an axis worked out
from the rest pose, the thumb towards the middle finger. In every clip the hands hold sword
and shield. Poses on the ground are lowered until the lowest sole point (heel, toe, both
sides of the ball, taken from the mesh) touches the floor.

**Previews** (`Saved/ArtPreview/humanoid_*.png`): each preset in the rest pose (turnaround)
and close to the face; the body without clothes; hand open, fist and grip; deformation
poses (`squat`, `reach`) per preset and with legs at 80 %; key frames of every clip.

**In the game.** `Scripts/build_art.sh humanoid` builds and imports (`Content/Characters/
humanoid`). The game side as before: `UHydraAnimInstance` plays the clips; sword and shield
hang on `weapon_r` / `shield_l` (Blender's +Y of a bone is −Y in Unreal); `HydraBody
neutral|male|female` sets the morph targets; `HydraLegs <factor>` scales `thigh_l/r` along
the bone; `Scripts/shots_humanoid.sh` makes the comparison page.

**Open points:**
- No underwear on the bare body (the skin texture is nude; shirt and trousers cover it).
  If the game ever shows the body without them, paint simple underwear into the skin.
- The skin texture is a photo-based CC0 texture, not procedural – the decision "textures
  procedural only" (2026-11-20) is about our own textures; noted in the dev log.
- Retargeting from the UE mannequin needs an IK retargeter with a pose adjustment (A-pose,
  other bone axes).
- About 42 000 triangles per humanoid; LODs come with more characters.

## Animations by the human: the import path

1. Open `Art/<name>/<name>.blend` and **save it under another name** (for example
   `Art/slime/slime_animations.blend`) – the build script overwrites `<name>.blend`.
2. Animate the armature; one action per animation. An action named like a placeholder
   (Idle, Move, Windup, …) replaces it in the game. Set the frame range in the action
   (Manual Frame Range), 30 fps.
3. `blender -b Art/slime/slime_animations.blend --python Art/scripts/export_animations.py -- slime`
4. `Scripts/build_art.sh import`

The rig must stay the one from the build script (bone names, rest pose). Animations from
other tools (for example Cascadeur) can be imported into that .blend first, or exported as
FBX with the same skeleton into `Art/export/<name>/A_<name>_<Action>.fbx` and imported with
step 4.

## Export settings (fixed in M3, in `hydra_blender.py`)

- **Scale:** the Blender scene unit is the centimetre (unit scale 0.01) and the scripts
  work in centimetres; FBX with `apply_unit_scale=True`, `apply_scale_options='FBX_SCALE_NONE'`,
  `global_scale=1`. Checked on import: slime 83 × 83 × 46 cm in Blender and in Unreal,
  skeleton without a scale.
- **Axes:** models look along −Y in Blender; FBX `axis_forward='-Z'`, `axis_up='Y'`. They
  arrive looking along +Y in Unreal (like Mixamo characters); the game turns the mesh by
  −90° yaw. The origin is on the floor under the body.
- **Skeleton:** one root bone at the origin, `add_leaf_bones=False`,
  `use_armature_deform_only=True`, `armature_nodetype='NULL'`, bone axes Y / X.
- **Animations:** one FBX per action (armature only), `bake_anim_use_all_actions=False`,
  no NLA, no simplification, the action's frame range.
- **Import:** `FbxImportUI` through an `AssetImportTask`, `import_materials=False`; the
  material is built by the script. It must be flagged *used with skeletal mesh*
  (`set_material_usage`), otherwise a game build shows the grey default material.
- Collision meshes (`UCX_`): not needed yet – characters collide with their capsule.

## Answers from the M3 test

- **Can agents produce models that fit the look and have topology good enough for
  animation?** For simple shapes yes: the slime (a squashed sphere with a flat base, eyes,
  three bones, weights by height) is clean and deforms well, and it is playable. For a
  four-legged body only as a rough stand-in: the wolf pup (stick figure wrapped by the Skin
  modifier, 424 quads, no triangles or n-gons, 15 bones, weights by distance) bends at every
  joint without tearing, but it has no modelled face, ears or paws and reads as "a small
  dog shape", not as a character. Good enough to test a rig and the pipeline, not good
  enough to ship. A characterful wolf needs either much more script work per model
  (profiles, loops placed by hand in code) or a human sculpting on top of the generated base.
- **How long does a simple form take compared with a complex one?** Slime: one script of
  about 230 lines, right on the second run. Wolf pup: about the same amount of script, but
  three runs, and the result is much further from "done". The cost is not the script, it
  is that shape quality cannot be judged from numbers – every step needs a look at a
  render.
- **Which steps need a human?** None for the pipeline (no intervention). Judging the look
  and designing animations stay with the human, as decided.
- **Procedural textures:** work headlessly (shader nodes baked through an emission shader,
  512 px for the slime, 1024 px for the wolf pup, a few seconds). Fine for a base coat;
  the look is decided in M5.
- **Things that went wrong on the way:** the Skin modifier left legs as separate tubes when
  a joint point sat inside the body volume (fixed by branching the legs directly off chest
  and pelvis); the sphere needs its pole and seam vertices welded; the first imported
  material was grey in the game (usage flag, see above); the model of *other* players'
  characters floated until the mesh offset was cached for network smoothing.
