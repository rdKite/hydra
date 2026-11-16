# Blender pipeline (by script)

Models, rigs and animations are made by agents with Blender Python scripts – no manual
steps, repeatable. First used in M3 (slime playable, wolf pup as a pipeline test).

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

## Export settings (to verify and fix in M3)

- Scale: Blender works in metres, Unreal in centimetres – set it once in the export script
  and check the imported size against a 1.8 m reference.
- Axes: Blender and Unreal are both Z-up; set forward axis in the export script and check
  the facing after import.
- Skeleton: a clean root bone, no extra leaf bones (`add_leaf_bones=False`).
- Collision meshes with the prefix `UCX_`.
- Record the final settings here and in the export script once they work.

## Questions for the M3 test

- Can agents produce models that fit the look and have topology good enough for animation?
- How long does a simple form (slime) take compared with a complex one (wolf pup)?
- Which steps need a human, if any? (→ interventions in the dev log)
