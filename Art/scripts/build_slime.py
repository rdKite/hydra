"""Builds the slime: model, rig, procedural texture, placeholder animations, FBX export.

  blender -b --python Art/scripts/build_slime.py            (or: Scripts/build_art.sh slime)

Writes Art/slime/slime.blend (source for animating by hand), Art/slime/T_slime_color.png,
Art/export/slime/*.fbx (build products, imported by Scripts/import_art.py) and preview
renders in Saved/ArtPreview/.

The rig is the rig of the whole slime line: root (stays on the ground), body, top.
The animations here are placeholders made by script; the human's animations replace them
action by action (same action names, see docs/tech/blender-pipeline.md).
"""
import math
import os
import sys

import bpy
from mathutils import Vector

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import hydra_blender as hb  # noqa: E402

NAME = "slime"
SEGMENTS, RINGS = 28, 16
RADIUS = 40.0          # cm, widest point
HEIGHT_SCALE = 0.72
FLAT_BOTTOM = -17.0    # sphere z below this is flattened to the floor
EYES = [Vector((11.0, -30.0, 30.0)), Vector((-11.0, -30.0, 30.0))]
EYE_RADIUS = 5.0


def body_point(ring, segment):
    """Point of the blob: a sphere, squashed, a little wider at the bottom, flat underneath."""
    polar = math.pi * ring / RINGS
    azimuth = 2.0 * math.pi * segment / SEGMENTS
    z = math.cos(polar) * RADIUS * HEIGHT_SCALE
    widen = 1.0 + 0.12 * hb.smoothstep(10.0, -20.0, z)
    radius = math.sin(polar) * RADIUS * widen
    return Vector((math.cos(azimuth) * radius, math.sin(azimuth) * radius, max(z, FLAT_BOTTOM) - FLAT_BOTTOM))


def sphere(center, radius, segments, rings):
    verts, faces = [], []
    for ring in range(rings + 1):
        polar = math.pi * ring / rings
        for segment in range(segments):
            azimuth = 2.0 * math.pi * segment / segments
            verts.append(center + Vector((math.cos(azimuth) * math.sin(polar), math.sin(azimuth) * math.sin(polar), math.cos(polar))) * radius)
    for ring in range(rings):
        for segment in range(segments):
            a = ring * segments + segment
            b = ring * segments + (segment + 1) % segments
            faces.append((a, b, b + segments, a + segments))
    return verts, faces


def build_mesh():
    verts, faces, uvs, eye_flags = [], [], [], []
    # Body: a grid of rings x segments (the seam column is doubled so the UVs can wrap cleanly).
    for ring in range(RINGS + 1):
        for segment in range(SEGMENTS + 1):
            verts.append(body_point(ring, segment % SEGMENTS))
            uvs.append((segment / SEGMENTS, 0.12 + 0.88 * (1.0 - ring / RINGS)))
            eye_flags.append(0.0)
    row = SEGMENTS + 1
    for ring in range(RINGS):
        for segment in range(SEGMENTS):
            a = ring * row + segment
            faces.append((a, a + row, a + row + 1, a + 1))
    # Eyes: two small spheres on the front.
    for index, center in enumerate(EYES):
        eye_verts, eye_faces = sphere(center, EYE_RADIUS, 10, 6)
        offset = len(verts)
        for i, v in enumerate(eye_verts):
            verts.append(v)
            uvs.append((0.02 + 0.05 * index + 0.04 * (i % 10) / 10.0, 0.02 + 0.08 * (i // 10) / 6.0))
            eye_flags.append(1.0)
        faces.extend(tuple(offset + i for i in face) for face in eye_faces)

    obj = hb.new_mesh_object("SK_" + NAME, verts, faces)
    mesh = obj.data
    uv_layer = mesh.uv_layers.new(name="UVMap")
    for loop in mesh.loops:
        uv_layer.data[loop.index].uv = uvs[loop.vertex_index]
    eye_attribute = mesh.attributes.new(name="eye", type='FLOAT', domain='POINT')
    for i, flag in enumerate(eye_flags):
        eye_attribute.data[i].value = flag
    hb.weld(obj)   # closes the poles and the seam column; the UVs stay per face corner
    hb.shade_smooth(obj)
    return obj


def build_rig(obj):
    arm = hb.build_armature("Armature", [
        ("root", None, (0, 0, 0), (0, 0, 8)),
        ("body", "root", (0, 0, 8), (0, 0, 32)),
        ("top", "body", (0, 0, 32), (0, 0, 58)),
    ])
    weights = {"root": {}, "body": {}, "top": {}}
    for vertex in obj.data.vertices:
        z = vertex.co.z
        w_root = 1.0 - hb.smoothstep(1.0, 14.0, z)
        w_top = hb.smoothstep(24.0, 50.0, z)
        w_body = max(0.0, 1.0 - w_root - w_top)
        total = w_root + w_body + w_top
        weights["root"][vertex.index] = w_root / total
        weights["body"][vertex.index] = w_body / total
        weights["top"][vertex.index] = w_top / total
    hb.bind(obj, arm, weights)
    return arm


def build_material(obj):
    """Gel: darker and bluer at the bottom, lighter at the top, with soft blotches; dark eyes."""
    material = hb.new_material("M_" + NAME)
    nodes, links = material.node_tree.nodes, material.node_tree.links

    coords = nodes.new('ShaderNodeTexCoord')
    separate = nodes.new('ShaderNodeSeparateXYZ')
    links.new(coords.outputs['Object'], separate.inputs['Vector'])
    height = nodes.new('ShaderNodeMapRange')
    height.inputs['From Min'].default_value = 0.0
    height.inputs['From Max'].default_value = 48.0
    links.new(separate.outputs['Z'], height.inputs['Value'])
    ramp = nodes.new('ShaderNodeValToRGB')
    ramp.color_ramp.elements[0].color = (0.02, 0.22, 0.20, 1.0)
    ramp.color_ramp.elements[1].color = (0.35, 0.85, 0.40, 1.0)
    links.new(height.outputs['Result'], ramp.inputs['Fac'])

    noise = nodes.new('ShaderNodeTexNoise')
    noise.inputs['Scale'].default_value = 0.09   # object coordinates are centimetres
    noise.inputs['Detail'].default_value = 3.0
    links.new(coords.outputs['Object'], noise.inputs['Vector'])
    blotches = nodes.new('ShaderNodeMix')
    blotches.data_type = 'RGBA'
    blotches.blend_type = 'OVERLAY'
    blotches.inputs['Factor'].default_value = 0.55
    links.new(ramp.outputs['Color'], blotches.inputs['A'])
    links.new(noise.outputs['Color'], blotches.inputs['B'])

    eye = nodes.new('ShaderNodeAttribute')
    eye.attribute_name = "eye"
    eyes = nodes.new('ShaderNodeMix')
    eyes.data_type = 'RGBA'
    eyes.inputs['B'].default_value = (0.01, 0.01, 0.015, 1.0)
    links.new(eye.outputs['Fac'], eyes.inputs['Factor'])
    links.new(blotches.outputs['Result'], eyes.inputs['A'])

    obj.data.materials.append(material)
    path = os.path.join(hb.ART_DIR, NAME, "T_%s_color.png" % NAME)
    hb.bake_color(obj, material, eyes.outputs['Result'], "T_%s_color" % NAME, 512, path)
    print("build_slime: texture " + path)


def build_animations(arm):
    """Placeholder actions, one per thing the game shows. Bone space: Y is up the bone."""
    forward = arm.data.bones["body"].matrix_local.to_3x3().inverted() @ Vector((0.0, -1.0, 0.0))

    def squash(frame, wide, tall, lunge=0.0, lift=0.0, top=None):
        hb.key(arm, "body", frame, location=tuple(forward * lunge + Vector((0.0, lift, 0.0))), scale=(wide, tall, wide))
        hb.key(arm, "top", frame, scale=top or (1.0, 1.0, 1.0))

    action = hb.new_action(arm, "Idle")             # loop: breathing
    squash(0, 1.0, 1.0)
    squash(20, 1.05, 0.93)
    squash(40, 1.0, 1.0)
    hb.set_action_range(action, 0, 40)

    action = hb.new_action(arm, "Move")             # loop: hop
    squash(0, 1.12, 0.82)
    squash(5, 0.90, 1.20, lift=6.0)
    squash(10, 1.0, 1.0, lift=3.0)
    squash(14, 1.12, 0.82)
    hb.set_action_range(action, 0, 14)

    action = hb.new_action(arm, "Windup")           # rise up before the slam (the telegraph)
    squash(0, 1.0, 1.0)
    squash(8, 0.82, 1.38, lunge=-6.0, top=(0.9, 1.15, 0.9))
    hb.set_action_range(action, 0, 8)

    action = hb.new_action(arm, "Charge")           # loop: trembling at full height
    squash(0, 0.82, 1.38, lunge=-6.0, top=(0.9, 1.15, 0.9))
    squash(3, 0.86, 1.33, lunge=-7.0, top=(0.9, 1.2, 0.9))
    squash(6, 0.82, 1.38, lunge=-6.0, top=(0.9, 1.15, 0.9))
    hb.set_action_range(action, 0, 6)

    action = hb.new_action(arm, "Strike")           # slam down and forward
    squash(0, 0.82, 1.38, lunge=-6.0, top=(0.9, 1.15, 0.9))
    squash(3, 1.40, 0.50, lunge=26.0)
    hb.set_action_range(action, 0, 3)

    action = hb.new_action(arm, "Recovery")
    squash(0, 1.40, 0.50, lunge=26.0)
    squash(10, 1.0, 1.0)
    hb.set_action_range(action, 0, 10)

    action = hb.new_action(arm, "Cast")             # puff up before the projectile
    squash(0, 1.0, 1.0)
    squash(8, 1.22, 1.12, top=(1.1, 1.1, 1.1))
    hb.set_action_range(action, 0, 8)

    action = hb.new_action(arm, "CastCharge")       # loop: swelling
    squash(0, 1.22, 1.12, top=(1.1, 1.1, 1.1))
    squash(5, 1.30, 1.18, top=(1.18, 1.15, 1.18))
    squash(10, 1.22, 1.12, top=(1.1, 1.1, 1.1))
    hb.set_action_range(action, 0, 10)

    action = hb.new_action(arm, "CastRelease")      # squeeze the projectile out
    squash(0, 1.22, 1.12, top=(1.1, 1.1, 1.1))
    squash(3, 0.85, 0.92, lunge=10.0)
    squash(8, 1.0, 1.0)
    hb.set_action_range(action, 0, 8)

    action = hb.new_action(arm, "Dodge")            # flat while it slides
    squash(0, 1.0, 1.0)
    squash(3, 1.25, 0.60)
    squash(11, 1.25, 0.60)
    squash(14, 1.0, 1.0)
    hb.set_action_range(action, 0, 14)

    action = hb.new_action(arm, "Death")            # melts
    squash(0, 1.0, 1.0)
    squash(15, 1.65, 0.16, top=(1.0, 0.4, 1.0))
    hb.set_action_range(action, 0, 15)

    arm.animation_data.action = bpy.data.actions["Idle"]


def main():
    hb.reset_scene()
    obj = build_mesh()
    arm = build_rig(obj)
    build_material(obj)
    build_animations(arm)

    size = obj.dimensions
    print("build_slime: size %.0f x %.0f x %.0f cm, %d vertices, %d faces" % (size.x, size.y, size.z, len(obj.data.vertices), len(obj.data.polygons)))

    hb.save_blend(os.path.join(NAME, NAME + ".blend"))
    hb.export_model(NAME, arm, [obj])
    hb.export_actions(NAME, arm)

    # Previews: rest pose and the two poses that deform most.
    arm.animation_data.action = bpy.data.actions["Idle"]
    bpy.context.scene.frame_set(0)
    hb.render_preview(NAME + "_rest", (0, 0, 22), 190)
    arm.animation_data.action = bpy.data.actions["Windup"]
    bpy.context.scene.frame_set(8)
    hb.render_preview(NAME + "_windup", (0, 0, 28), 190, views=(("front_left", -35.0),))
    arm.animation_data.action = bpy.data.actions["Strike"]
    bpy.context.scene.frame_set(3)
    hb.render_preview(NAME + "_strike", (0, 0, 20), 190, views=(("side", -90.0),))
    print("build_slime: done")


main()
