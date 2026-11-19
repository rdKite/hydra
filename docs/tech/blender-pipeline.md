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
- `Art/scripts/build_slime.py`, `build_wolf_pup.py`, `build_humanoid.py` – one script per model.
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

## Humanoid base model (M5, Blender part)

`Art/scripts/build_humanoid.py` builds the one base model for all humanoids (decisions of
2026-11-19). Run: `blender -b --python Art/scripts/build_humanoid.py` (about 70 s with
previews; `-- nopreview` skips them). `Scripts/build_art.sh humanoid` also works, but it then
runs the Unreal import as well, which belongs to the Unreal integration (see open points).

**Body.** A plain, stylised, gender-neutral stand-in, 179 cm, A-pose (arms 40° from the
body), no face, mitten hands. It is not built with the Skin modifier like the wolf pup:
the script writes a quad cage ring by ring (a torso lattice, 8-sided rings for arms and
legs with extra loops at shoulder, elbow, wrist, hip, knee and ankle, a 6-sided foot),
mirrors it and applies one Catmull-Clark level. Result: 2306 vertices, 2304 faces, all
quads, closed, exactly symmetric, 2 poles with more than 5 edges (at the crotch). The
build prints these numbers on every run.

**Weights** are set per cage ring (each ring knows its bones, e.g. the knee ring is half
thigh, half calf) and smoothed by the subdivision; at most 4 bones per vertex, normalised.
Weights by distance (as on the wolf pup) were not used: the arm hangs next to the trunk and
the thighs touch, so the nearest bone is often the wrong one. The buttocks follow the thigh
with only 40 % of their ring's share, otherwise a deep squat folds them into a sharp ridge.

**Skeleton** (25 bones; names of the Unreal mannequin, but Blender bone axes with Y along
the bone, not the mannequin's axes):

- `root` (at the origin) → `pelvis` → `spine_01` → `spine_02` → `spine_03` → `neck_01` → `head`
- `spine_03` → `clavicle_l/r` → `upperarm_l/r` → `lowerarm_l/r` → `hand_l/r`
- `pelvis` → `thigh_l/r` → `calf_l/r` → `foot_l/r` → `ball_l/r`
- `weapon_r`, child of `hand_r`: the grip in the palm, **+Y of the bone along the blade**
  (points forward in the rest pose).
- `shield_l`, child of `lowerarm_l`: on the outside of the forearm, **+Y of the bone out of
  the shield's face**, shield centre at the bone head.

No finger, twist or IK bones yet. Every bone is flagged as a deform bone (also `root` and
the two socket bones, which carry no weights), because the export keeps deform bones only.
The `.blend` holds a stand-in longsword (98 cm) and round shield (60 cm) parented to the
socket bones, so animations can be judged with them; they are **not** exported.

**Body presets.** The cage is built three times with other measures (shoulders, chest,
waist, hips, limb thickness, neck, jaw); the joints never move, so one skeleton and one set
of animations fit every preset. Shape keys `male` and `female`; the neutral preset is the
base shape. They are exported as FBX blend shapes (checked by re-importing the FBX:
`Basis`, `male`, `female`) and become morph targets in Unreal.

**Placeholder animations** (one action each, exported as `A_humanoid_<Name>.fbx`, 30 fps):
Idle (40 frames, loop), Move (16, loop, runs on the spot), Windup (8), Strike (4, a slash
from the right shoulder across to the left), Recovery (10), Charge (12, loop, sword drawn
back at the hip), **HeavyStrike** (4, a thrust with a lunge – the heavy attack is a stab; the
game falls back to Strike if a model has no HeavyStrike), Block (4, shield in front of the chest, body
lowered; the last frame is held), Dodge (14), Jump (12, loop while in the air), Death (16,
falls on its back). Poses with the feet on the ground are lowered by script until the
lowest sole point touches the floor (for the full leg length; with shorter legs the game
has to correct the height, see below).

**Previews** (`Saved/ArtPreview/humanoid_*.png`): each preset in the rest pose (front,
front-left, side, back); two deformation test poses for every preset (`squat`: hips and
knees at 95° / 115°; `reach`: an arm raised by 120°, a leg spread by 45°); the rest pose,
both test poses and Idle, Strike and Block with thigh and calf scaled to 80 % (`*_legs80`);
key frames of every clip with sword and shield.

**What the renders show.** Shoulders, elbows, hips and knees bend without tearing or
collapsing in all three presets; the knees keep their volume at 115°. At 80 % leg length
hips and knees sit right and the proportions read as stocky. The test scales `thigh` and
`calf` along the bone with `inherit_scale = NONE` on `calf` and `foot` (so the feet keep
their size) and lowers the whole figure until the soles touch the floor. Weak points: in
the deep squat belly and thigh press into each other and the buttocks still form a mild
point; the top of the shoulder shows a small bump when the arm is raised high; the presets
differ clearly but moderately.

**Two ways to shorten the legs, compared** (`humanoid_rest_legs80_*` vs.
`humanoid_rest_legs80_moved_*`): scaling `thigh` and `calf` along the bone shortens the skin
with them – clean. Only moving `calf` and `foot` closer (bone translations, no scale – what
`UHydraAnimInstance::ScaleLegs` does at the time of writing) leaves the thigh and calf skin
at full length, so it is pushed into the next segment: visible folds at the knees and above
the ankles at 80 %. Readable from a distance, but the bone scale is the better fit for this
mesh.

**Open points (for the Unreal integration and the human):**
- The humanoid is not in the default model list of `build_art.sh` yet.
- Leg length at runtime: consider scaling thigh and calf along their bone axis (Blender's
  +Y; which Unreal axis that becomes after the import has to be checked) instead of only
  moving calf and foot, with the children not inheriting the scale; either way the figure
  comes down by the lost leg length (about 17 cm at 80 %).
- The A-pose is the rest pose. Retargeting from the UE mannequin needs an IK retargeter with
  a pose adjustment (the bone axes differ).
- Hands are mittens; the sword grip is placed by the socket only. Fingers if the human's
  animations need them.

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
