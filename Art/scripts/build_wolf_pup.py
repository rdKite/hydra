"""Builds the wolf pup as a pipeline test: model, rig, procedural texture, FBX export.

  blender -b --python Art/scripts/build_wolf_pup.py         (or: Scripts/build_art.sh wolf_pup)

Not playable in Phase 1. The questions it answers (docs/tech/blender-pipeline.md): can a
script produce a four-legged model whose topology is good enough for animation, and how
much more work is it than the slime?

Method: the body is drawn as a stick figure (points with a thickness, joined by edges) and
Blender's Skin modifier wraps it in quads; a Subdivision level smooths it. The same stick
figure gives the bones, so mesh and rig always fit. Weights come from the distance to the
bones. Writes Art/wolf_pup/wolf_pup.blend, the texture, Art/export/wolf_pup/*.fbx and
preview renders (rest pose and a test pose that bends every joint).
"""
import os
import sys

import bpy
from mathutils import Vector

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import hydra_blender as hb  # noqa: E402

NAME = "wolf_pup"

# Stick figure in centimetres, facing -Y: name -> (position, thickness radius).
POINTS = {
    "pelvis": ((0, 16, 27), 7.0),
    "spine": ((0, 4, 29), 7.5),
    "chest": ((0, -9, 29), 8.0),
    "neck": ((0, -17, 34), 5.5),
    "head": ((0, -24, 38), 6.5),
    "snout": ((0, -33, 35.5), 3.2),
    "tail1": ((0, 25, 28), 3.0),
    "tail2": ((0, 35, 24), 1.8),
    "ear.L": ((5.0, -21, 47.5), 2.6),
    "ear.R": ((-5.0, -21, 47.5), 2.6),
    "shoulder.L": ((6, -9, 21), 4.2),
    "elbow.L": ((6, -8, 11.5), 2.8),
    "paw.L": ((6, -10, 2.6), 2.6),
    "shoulder.R": ((-6, -9, 21), 4.2),
    "elbow.R": ((-6, -8, 11.5), 2.8),
    "paw.R": ((-6, -10, 2.6), 2.6),
    "hip.L": ((6.5, 16, 20), 4.8),
    "knee.L": ((6.5, 19, 11), 3.0),
    "hindpaw.L": ((6.5, 17, 2.6), 2.6),
    "hip.R": ((-6.5, 16, 20), 4.8),
    "knee.R": ((-6.5, 19, 11), 3.0),
    "hindpaw.R": ((-6.5, 17, 2.6), 2.6),
}
EDGES = [
    ("pelvis", "spine"), ("spine", "chest"), ("chest", "neck"), ("neck", "head"), ("head", "snout"),
    ("pelvis", "tail1"), ("tail1", "tail2"), ("head", "ear.L"), ("head", "ear.R"),
    # Legs branch straight off chest and pelvis: with an extra point inside the body (the
    # shoulder, the hip) the Skin modifier left the legs as separate, unconnected tubes.
    ("chest", "elbow.L"), ("elbow.L", "paw.L"),
    ("chest", "elbow.R"), ("elbow.R", "paw.R"),
    ("pelvis", "knee.L"), ("knee.L", "hindpaw.L"),
    ("pelvis", "knee.R"), ("knee.R", "hindpaw.R"),
]
# Bones: (name, parent, from point, to point). One rig for the whole wolf line.
BONES = [
    ("pelvis", "root", "pelvis", "spine"),
    ("spine", "pelvis", "spine", "chest"),
    ("neck", "spine", "chest", "neck"),
    ("head", "neck", "neck", "snout"),
    ("tail1", "pelvis", "pelvis", "tail1"),
    ("tail2", "tail1", "tail1", "tail2"),
    ("front_upper.L", "spine", "shoulder.L", "elbow.L"),
    ("front_lower.L", "front_upper.L", "elbow.L", "paw.L"),
    ("front_upper.R", "spine", "shoulder.R", "elbow.R"),
    ("front_lower.R", "front_upper.R", "elbow.R", "paw.R"),
    ("hind_upper.L", "pelvis", "hip.L", "knee.L"),
    ("hind_lower.L", "hind_upper.L", "knee.L", "hindpaw.L"),
    ("hind_upper.R", "pelvis", "hip.R", "knee.R"),
    ("hind_lower.R", "hind_upper.R", "knee.R", "hindpaw.R"),
]


def build_mesh():
    # Only the points that are joined by edges become mesh (shoulder and hip are bone joints only).
    names = [name for name in POINTS if any(name in edge for edge in EDGES)]
    index = {name: i for i, name in enumerate(names)}
    mesh = bpy.data.meshes.new("stick_figure")
    mesh.from_pydata([POINTS[n][0] for n in names], [(index[a], index[b]) for a, b in EDGES], [])
    mesh.update()
    stick = bpy.data.objects.new("stick_figure", mesh)
    bpy.context.scene.collection.objects.link(stick)

    skin = stick.modifiers.new("Skin", 'SKIN')
    skin.use_smooth_shade = True
    for i, name in enumerate(names):
        radius = POINTS[name][1]
        mesh.skin_vertices[0].data[i].radius = (radius, radius)
        mesh.skin_vertices[0].data[i].use_root = name == "pelvis"
    subdivision = stick.modifiers.new("Subdivision", 'SUBSURF')
    subdivision.levels = 1

    # Bake the modifiers into a plain mesh (what gets rigged and exported).
    depsgraph = bpy.context.evaluated_depsgraph_get()
    final_mesh = bpy.data.meshes.new_from_object(stick.evaluated_get(depsgraph))
    final_mesh.name = "SK_" + NAME
    obj = bpy.data.objects.new("SK_" + NAME, final_mesh)
    bpy.context.scene.collection.objects.link(obj)
    bpy.data.objects.remove(stick, do_unlink=True)
    hb.shade_smooth(obj)

    # UVs: automatic islands.
    hb.set_active(obj)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.02)
    bpy.ops.object.mode_set(mode='OBJECT')
    return obj


def build_rig(obj):
    bones = [("root", None, (0, 0, 0), (0, 8, 0))]
    for name, parent, start, end in BONES:
        bones.append((name, parent, POINTS[start][0], POINTS[end][0]))
    arm = hb.build_armature("Armature", bones)
    hb.bind(obj, arm, hb.distance_weights(obj, arm, [b[0] for b in BONES]))
    return arm


def build_material(obj):
    """Fur as a base coat: grey-brown back, lighter belly and legs, soft variation, dark nose and paws."""
    material = hb.new_material("M_" + NAME)
    nodes, links = material.node_tree.nodes, material.node_tree.links

    coords = nodes.new('ShaderNodeTexCoord')
    separate = nodes.new('ShaderNodeSeparateXYZ')
    links.new(coords.outputs['Object'], separate.inputs['Vector'])

    height = nodes.new('ShaderNodeMapRange')
    height.inputs['From Min'].default_value = 14.0
    height.inputs['From Max'].default_value = 34.0
    links.new(separate.outputs['Z'], height.inputs['Value'])
    coat = nodes.new('ShaderNodeValToRGB')
    coat.color_ramp.elements[0].color = (0.62, 0.58, 0.52, 1.0)   # belly, legs
    coat.color_ramp.elements[1].color = (0.20, 0.19, 0.20, 1.0)   # back
    links.new(height.outputs['Result'], coat.inputs['Fac'])

    noise = nodes.new('ShaderNodeTexNoise')
    noise.inputs['Scale'].default_value = 0.35
    noise.inputs['Detail'].default_value = 4.0
    links.new(coords.outputs['Object'], noise.inputs['Vector'])
    varied = nodes.new('ShaderNodeMix')
    varied.data_type = 'RGBA'
    varied.blend_type = 'SOFT_LIGHT'
    varied.inputs['Factor'].default_value = 0.6
    links.new(coat.outputs['Color'], varied.inputs['A'])
    links.new(noise.outputs['Color'], varied.inputs['B'])

    # Dark paws (near the floor) and a dark nose (front tip).
    paws = nodes.new('ShaderNodeMapRange')
    paws.inputs['From Min'].default_value = 4.5
    paws.inputs['From Max'].default_value = 2.5
    links.new(separate.outputs['Z'], paws.inputs['Value'])
    nose = nodes.new('ShaderNodeMapRange')
    nose.inputs['From Min'].default_value = -33.5
    nose.inputs['From Max'].default_value = -35.5
    links.new(separate.outputs['Y'], nose.inputs['Value'])
    dark_mask = nodes.new('ShaderNodeMath')
    dark_mask.operation = 'MAXIMUM'
    links.new(paws.outputs['Result'], dark_mask.inputs[0])
    links.new(nose.outputs['Result'], dark_mask.inputs[1])
    final = nodes.new('ShaderNodeMix')
    final.data_type = 'RGBA'
    final.inputs['B'].default_value = (0.03, 0.025, 0.025, 1.0)
    links.new(dark_mask.outputs['Value'], final.inputs['Factor'])
    links.new(varied.outputs['Result'], final.inputs['A'])

    obj.data.materials.append(material)
    path = os.path.join(hb.ART_DIR, NAME, "T_%s_color.png" % NAME)
    hb.bake_color(obj, material, final.outputs['Result'], "T_%s_color" % NAME, 1024, path)
    print("build_wolf_pup: texture " + path)


def build_test_pose(arm):
    """One action that bends every joint, to judge how the mesh deforms. Not a game animation."""
    action = hb.new_action(arm, "TestPose")
    rest = dict(rotation_deg=(0, 0, 0))
    bent = {
        "front_upper.L": (35, 0, 0), "front_lower.L": (-45, 0, 0),
        "front_upper.R": (-30, 0, 0), "front_lower.R": (25, 0, 0),
        "hind_upper.L": (-30, 0, 0), "hind_lower.L": (40, 0, 0),
        "hind_upper.R": (30, 0, 0), "hind_lower.R": (-35, 0, 0),
        "neck": (-25, 0, 0), "head": (30, 0, 15),
        "tail1": (40, 0, 0), "tail2": (35, 0, 0),
        "spine": (0, 0, 12),
    }
    for bone in bent:
        hb.key(arm, bone, 0, **rest)
        hb.key(arm, bone, 10, rotation_deg=bent[bone])
        hb.key(arm, bone, 20, **rest)
    hb.set_action_range(action, 0, 20)


def report_topology(obj):
    mesh = obj.data
    quads = sum(1 for p in mesh.polygons if len(p.vertices) == 4)
    tris = sum(1 for p in mesh.polygons if len(p.vertices) == 3)
    ngons = len(mesh.polygons) - quads - tris
    valence = {}
    for edge in mesh.edges:
        for v in edge.vertices:
            valence[v] = valence.get(v, 0) + 1
    poles = sum(1 for count in valence.values() if count > 5)
    size = obj.dimensions
    print("build_wolf_pup: size %.0f x %.0f x %.0f cm, %d vertices, %d faces (%d quads, %d triangles, %d n-gons), %d vertices with more than 5 edges"
          % (size.x, size.y, size.z, len(mesh.vertices), len(mesh.polygons), quads, tris, ngons, poles))


def main():
    hb.reset_scene()
    obj = build_mesh()
    arm = build_rig(obj)
    build_material(obj)
    build_test_pose(arm)
    report_topology(obj)

    hb.save_blend(os.path.join(NAME, NAME + ".blend"))
    hb.export_model(NAME, arm, [obj])
    hb.export_actions(NAME, arm)

    arm.animation_data.action = bpy.data.actions["TestPose"]
    bpy.context.scene.frame_set(0)
    hb.render_preview(NAME + "_rest", (0, 0, 22), 150)
    bpy.context.scene.frame_set(10)
    hb.render_preview(NAME + "_testpose", (0, 0, 22), 150, views=(("front_left", -35.0), ("side", -90.0)))
    print("build_wolf_pup: done")


main()
