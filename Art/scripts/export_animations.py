"""Exports every action of a .blend as an animation FBX for Unreal - the path for
animations made by the human.

  blender -b Art/slime/slime_animations.blend --python Art/scripts/export_animations.py -- slime
  Scripts/build_art.sh import

The .blend must contain the armature of that model (start from Art/<name>/<name>.blend,
save it under another name so the build script does not overwrite your work, animate).
Every action becomes Art/export/<name>/A_<name>_<Action>.fbx and, after the import,
/Game/Characters/<name>/A_<name>_<Action>. An action with the name of a placeholder
(Idle, Move, Windup, ...) replaces it in the game; set its frame range in the action
(Manual Frame Range), otherwise the keyed range is used.
"""
import os
import sys

import bpy

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import hydra_blender as hb  # noqa: E402

args = hb.script_args()
if len(args) != 1:
    raise SystemExit("usage: blender -b <file.blend> --python export_animations.py -- <model name>")
armatures = [obj for obj in bpy.context.scene.objects if obj.type == 'ARMATURE']
if len(armatures) != 1:
    raise SystemExit("export_animations: expected exactly one armature in the scene, found %d" % len(armatures))
if not bpy.data.actions:
    raise SystemExit("export_animations: the file has no actions")
exported = hb.export_actions(args[0], armatures[0])
print("export_animations: %d actions exported for %s" % (len(exported), args[0]))
