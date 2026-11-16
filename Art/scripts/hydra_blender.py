"""Shared helpers for the hydra Blender scripts (models, rigs, textures, export).

Everything here runs headless:  blender -b --python Art/scripts/<script>.py
See docs/tech/blender-pipeline.md for the rules and the fixed export settings.

Conventions
- Units: the scene unit is the centimetre (unit scale 0.01), so numbers in the scripts are
  centimetres and arrive in Unreal 1:1 (no scale on the skeleton).
- Facing: models look along -Y in Blender (front view shows the face). With the fixed FBX
  axes below they look along +Y in Unreal; the game turns the mesh by -90 degrees yaw.
- Feet on the ground: the origin is on the floor under the body.
- Skeleton: one clean root bone at the origin, no leaf bones.
"""
import math
import os
import sys

import bpy
from mathutils import Vector

ART_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXPORT_DIR = os.path.join(ART_DIR, "export")
PREVIEW_DIR = os.path.join(os.path.dirname(ART_DIR), "Saved", "ArtPreview")
FPS = 30


def script_args():
    """Arguments after '--' on the Blender command line."""
    return sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def reset_scene():
    """Empty scene in centimetres at 30 fps."""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 0.01
    scene.render.fps = FPS
    return scene


def deselect_all():
    bpy.context.view_layer.update()   # objects removed a moment ago must be gone from the list
    for other in bpy.context.view_layer.objects:
        if other is not None:
            other.select_set(False)


def set_active(obj):
    deselect_all()
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def new_material(name):
    """Material with a node tree that holds only the output node."""
    material = bpy.data.materials.new(name)
    if material.node_tree is None:
        material.use_nodes = True   # older Blender versions; newer ones always have a tree
    for node in list(material.node_tree.nodes):
        if node.type != 'OUTPUT_MATERIAL':
            material.node_tree.nodes.remove(node)
    return material


def new_mesh_object(name, verts, faces):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def weld(obj, distance=0.001):
    """Merges vertices that sit on the same spot (poles, UV seams built as doubled columns)."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=distance)
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()


def shade_smooth(obj):
    for poly in obj.data.polygons:
        poly.use_smooth = True


def smoothstep(edge0, edge1, x):
    t = max(0.0, min(1.0, (x - edge0) / (edge1 - edge0)))
    return t * t * (3.0 - 2.0 * t)


# --- Rig -----------------------------------------------------------------------------------

def build_armature(name, bones):
    """bones: list of (name, parent name or None, head, tail) in centimetres. Returns the armature object."""
    data = bpy.data.armatures.new(name)
    arm = bpy.data.objects.new(name, data)
    bpy.context.scene.collection.objects.link(arm)
    set_active(arm)
    bpy.ops.object.mode_set(mode='EDIT')
    for bone_name, parent, head, tail in bones:
        bone = data.edit_bones.new(bone_name)
        bone.head = Vector(head)
        bone.tail = Vector(tail)
        if parent:
            bone.parent = data.edit_bones[parent]
    bpy.ops.object.mode_set(mode='OBJECT')
    return arm


def bind(obj, arm, weights):
    """weights: {bone name: {vertex index: weight}}. Parents obj to arm with an armature modifier."""
    for bone_name, per_vertex in weights.items():
        group = obj.vertex_groups.new(name=bone_name)
        for index, weight in per_vertex.items():
            if weight > 0.0001:
                group.add([index], weight, 'REPLACE')
    obj.parent = arm
    modifier = obj.modifiers.new("Armature", 'ARMATURE')
    modifier.object = arm


def distance_weights(obj, arm, bone_names, power=4.0, max_influences=3):
    """Weights from the distance of every vertex to the bones (segments), the nearest few bones win."""
    segments = {}
    for name in bone_names:
        bone = arm.data.bones[name]
        segments[name] = (bone.head_local.copy(), bone.tail_local.copy())
    weights = {name: {} for name in bone_names}
    for vertex in obj.data.vertices:
        scored = []
        for name, (head, tail) in segments.items():
            axis = tail - head
            t = max(0.0, min(1.0, (vertex.co - head).dot(axis) / max(axis.length_squared, 1e-9)))
            distance = (vertex.co - (head + axis * t)).length
            scored.append((1.0 / max(distance, 0.05) ** power, name))
        scored.sort(reverse=True)
        best = scored[:max_influences]
        total = sum(score for score, _ in best)
        for score, name in best:
            weights[name][vertex.index] = score / total
    return weights


# --- Placeholder animations ----------------------------------------------------------------

def new_action(arm, name):
    """Fresh action on the armature with every pose bone back in rest pose."""
    if arm.animation_data is None:
        arm.animation_data_create()
    action = bpy.data.actions.new(name)
    action.use_fake_user = True
    arm.animation_data.action = action
    for bone in arm.pose.bones:
        bone.location = (0.0, 0.0, 0.0)
        bone.rotation_mode = 'XYZ'
        bone.rotation_euler = (0.0, 0.0, 0.0)
        bone.scale = (1.0, 1.0, 1.0)
    return action


def key(arm, bone_name, frame, location=None, rotation_deg=None, scale=None):
    """Keyframe a pose bone (bone space: Y is along the bone)."""
    bone = arm.pose.bones[bone_name]
    if location is not None:
        bone.location = location
        bone.keyframe_insert("location", frame=frame)
    if rotation_deg is not None:
        bone.rotation_euler = tuple(math.radians(a) for a in rotation_deg)
        bone.keyframe_insert("rotation_euler", frame=frame)
    if scale is not None:
        bone.scale = scale
        bone.keyframe_insert("scale", frame=frame)


def set_action_range(action, first, last):
    action.use_frame_range = True
    action.frame_start = first
    action.frame_end = last


# --- Procedural texture --------------------------------------------------------------------

def bake_color(obj, material, color_socket, image_name, size, path):
    """Bakes the colour that feeds color_socket's node output into a PNG, using the object's UVs.

    The colour goes through an emission shader for the bake (no lighting in the texture);
    afterwards the material uses the baked image as its base colour.
    """
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 4
    tree = material.node_tree
    nodes, links = tree.nodes, tree.links

    image = bpy.data.images.new(image_name, size, size, alpha=False)
    target = nodes.new('ShaderNodeTexImage')
    target.image = image
    output = next(n for n in nodes if n.type == 'OUTPUT_MATERIAL')
    emission = nodes.new('ShaderNodeEmission')
    links.new(color_socket, emission.inputs['Color'])
    links.new(emission.outputs['Emission'], output.inputs['Surface'])

    set_active(obj)
    nodes.active = target
    bpy.ops.object.bake(type='EMIT', margin=8)

    os.makedirs(os.path.dirname(path), exist_ok=True)
    image.filepath_raw = path
    image.file_format = 'PNG'
    image.save()

    # The model now carries the baked texture (what the game gets), not the node network.
    shader = nodes.new('ShaderNodeBsdfPrincipled')
    links.new(target.outputs['Color'], shader.inputs['Base Color'])
    links.new(shader.outputs['BSDF'], output.inputs['Surface'])
    nodes.remove(emission)
    return image


# --- Export --------------------------------------------------------------------------------

# The fixed FBX settings for Unreal (recorded in docs/tech/blender-pipeline.md).
FBX_SETTINGS = dict(
    use_selection=True,
    global_scale=1.0,
    apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_NONE',
    axis_forward='-Z',
    axis_up='Y',
    use_space_transform=True,
    bake_space_transform=False,
    object_types={'ARMATURE', 'MESH'},
    use_mesh_modifiers=True,
    mesh_smooth_type='FACE',
    add_leaf_bones=False,
    primary_bone_axis='Y',
    secondary_bone_axis='X',
    use_armature_deform_only=True,
    armature_nodetype='NULL',
    bake_anim_use_all_bones=True,
    bake_anim_use_nla_strips=False,
    bake_anim_use_all_actions=False,
    bake_anim_force_startend_keying=True,
    bake_anim_simplify_factor=0.0,
    path_mode='AUTO',
)


def export_model(name, arm, meshes):
    """<export>/<name>/SK_<name>.fbx: mesh and skeleton, no animation."""
    folder = os.path.join(EXPORT_DIR, name)
    os.makedirs(folder, exist_ok=True)
    deselect_all()
    arm.select_set(True)
    for mesh in meshes:
        mesh.select_set(True)
    bpy.context.view_layer.objects.active = arm
    saved = arm.animation_data.action if arm.animation_data else None
    if arm.animation_data:
        arm.animation_data.action = None
    for bone in arm.pose.bones:
        bone.location = (0.0, 0.0, 0.0)
        bone.rotation_euler = (0.0, 0.0, 0.0)
        bone.scale = (1.0, 1.0, 1.0)
    path = os.path.join(folder, "SK_%s.fbx" % name)
    bpy.ops.export_scene.fbx(filepath=path, bake_anim=False, **FBX_SETTINGS)
    if arm.animation_data:
        arm.animation_data.action = saved
    print("hydra_blender: exported " + path)
    return path


def export_actions(name, arm):
    """<export>/<name>/A_<name>_<action>.fbx for every action in the file: skeleton and animation only.

    This is also the path for animations made by the human: open or create a .blend with
    this armature, make actions, run Art/scripts/export_animations.py on it.
    """
    folder = os.path.join(EXPORT_DIR, name)
    os.makedirs(folder, exist_ok=True)
    deselect_all()
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    if arm.animation_data is None:
        arm.animation_data_create()
    settings = dict(FBX_SETTINGS)
    settings['object_types'] = {'ARMATURE'}
    exported = []
    for action in bpy.data.actions:
        arm.animation_data.action = action
        first, last = (int(action.frame_start), int(action.frame_end)) if action.use_frame_range else (int(action.frame_range[0]), int(action.frame_range[1]))
        bpy.context.scene.frame_start = first
        bpy.context.scene.frame_end = last
        path = os.path.join(folder, "A_%s_%s.fbx" % (name, action.name))
        bpy.ops.export_scene.fbx(filepath=path, bake_anim=True, **settings)
        exported.append(path)
        print("hydra_blender: exported %s (frames %d-%d)" % (path, first, last))
    return exported


def save_blend(relative_path):
    path = os.path.join(ART_DIR, relative_path)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=path, compress=True)
    print("hydra_blender: saved " + path)


# --- Preview render ------------------------------------------------------------------------

def render_preview(name, look_at, distance, size=(900, 600), views=(("front_left", -35.0), ("side", -90.0), ("back_right", 145.0))):
    """Small Cycles renders into Saved/ArtPreview/ so a human can judge shape and deformation."""
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 24
    scene.render.resolution_x, scene.render.resolution_y = size
    scene.render.image_settings.file_format = 'PNG'
    scene.view_settings.view_transform = 'Standard'   # colours as baked, not filmic

    world = bpy.data.worlds.new("PreviewWorld")
    if world.node_tree is None:
        world.use_nodes = True
    background = next((n for n in world.node_tree.nodes if n.type == 'BACKGROUND'), None)
    if background is not None:
        background.inputs[0].default_value = (0.55, 0.6, 0.7, 1.0)
        background.inputs[1].default_value = 0.6
    scene.world = world

    sun_data = bpy.data.lights.new("PreviewSun", 'SUN')
    sun_data.energy = 2.0
    sun = bpy.data.objects.new("PreviewSun", sun_data)
    sun.rotation_euler = (math.radians(50), 0.0, math.radians(30))
    scene.collection.objects.link(sun)

    floor = new_mesh_object("PreviewFloor", [(-400, -400, 0), (400, -400, 0), (400, 400, 0), (-400, 400, 0)], [(0, 1, 2, 3)])

    cam_data = bpy.data.cameras.new("PreviewCamera")
    cam_data.lens = 50
    cam_data.clip_end = 100000
    cam = bpy.data.objects.new("PreviewCamera", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam

    os.makedirs(PREVIEW_DIR, exist_ok=True)
    paths = []
    target = Vector(look_at)
    for view_name, angle_deg in views:
        angle = math.radians(angle_deg)
        # angle 0 = camera in front of the model (model faces -Y), turning anticlockwise seen from above
        cam.location = target + Vector((math.sin(angle) * distance, -math.cos(angle) * distance, distance * 0.35))
        cam.rotation_euler = (target - cam.location).to_track_quat('-Z', 'Y').to_euler()
        scene.render.filepath = os.path.join(PREVIEW_DIR, "%s_%s.png" % (name, view_name))
        bpy.ops.render.render(write_still=True)
        paths.append(scene.render.filepath)
        print("hydra_blender: preview " + scene.render.filepath)

    for obj in (floor, cam, sun):
        bpy.data.objects.remove(obj, do_unlink=True)
    return paths
