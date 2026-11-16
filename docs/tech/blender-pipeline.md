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
- **Materials** don't transfer 1:1: textures are exported, the look is made in Unreal (still
  open, decided in M5).

## Textures (procedural, by script)

- Materials are built from Blender shader nodes by Python (noise, gradients, edge wear,
  color variation, vertex colors) and **baked to textures headlessly** (PNG, Git LFS). The
  script is the source; textures are a build product of it.
- Good for stone, wood, earth, slime gel, skin and fur as a base. The more stylized the
  look, the fewer textures are needed – much can come from vertex colors, gradient maps and
  the shader in Unreal.
- Workflow: M3 tries it on slime and wolf pup → M5 decides the look and shows how much
  texture work it needs → only if hand-painted detail is needed, add AI image generation
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
- `Art/scripts/build_slime.py`, `build_wolf_pup.py` – one script per model.
- `Art/<name>/<name>.blend` and `T_<name>_color.png` – written by the build script (LFS).
- `Art/export/<name>/*.fbx` – build products, not in git.
- `Scripts/import_art.py` – Unreal Python commandlet: `Content/Characters/<name>/`
  (`SK_<name>`, skeleton, `T_<name>_color`, `M_<name>`, `A_<name>_<Action>`).
- `Saved/ArtPreview/*.png` – small renders of every build to judge shape and deformation.

The game finds a model by name: a form `<id>` with `/Game/Characters/<id>/SK_<id>` is shown
with it, otherwise with the placeholder body. Animations are looked up as `A_<id>_<Name>`
with the names Idle, Move, Windup, Charge, Strike, Recovery, Cast, CastCharge, CastRelease,
Dodge, Death. Clips of actions are time-scaled to the action's duration from
`values.json`, so their length in Blender does not change the gameplay timing.

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
