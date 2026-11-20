"""Humanoid base model: one skeleton and one mesh for every humanoid (hero and humanoid enemies).

Run:  blender -b --python Art/scripts/build_humanoid.py [-- nopreview]
Needs the MPFB2 extension (MakeHuman for Blender) and the MakeHuman system assets in Blender:
Scripts/setup_mpfb.sh installs both once per machine.

Output
  Art/humanoid/humanoid.blend          the working file (stand-in sword and shield included)
  Art/humanoid/T_humanoid_<slot>.png   one texture per material slot (see materials.json)
  Art/export/humanoid/SK_humanoid.fbx  mesh (morph targets male, female) and skeleton
  Art/export/humanoid/A_humanoid_*.fbx the placeholder animations
  Saved/ArtPreview/humanoid_*.png      renders to judge shape, deformation and poses

How the body is made
- MPFB builds a realistic human from the MakeHuman base mesh (CC0) with its macro sliders
  (gender, muscle, weight, proportions ...) and fits its "game engine" rig to it (bone names
  of the Unreal mannequin, with fingers). Skin, eyes, eyebrows and eyelashes are MakeHuman
  assets (CC0). No clothes and no hair on the body: those come as equipment and hair styles.
- A plain shirt and trousers are cut from the body by script: the same vertices and bone
  weights as the skin under them, smoothed (no anatomy shows through), pushed out loosely,
  given cloth thickness. They are their own material slots, so the game can tint them.
- Body presets: the neutral body is the base, `male` and `female` are shape keys (morph
  targets in Unreal). They are built with other macro values and scaled to the neutral
  height, so one skeleton fits all three; the game sets the height by scaling.
"""
import math
import os
import shutil
import sys

import addon_utils
import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import hydra_blender as hb  # noqa: E402

NAME = "humanoid"
OUT_DIR = os.path.join(hb.ART_DIR, NAME)
MPFB = "bl_ext.user_default.mpfb"

# Macro sliders of MakeHuman (0..1). Gender per preset; the rest is shared.
PRESETS = {"neutral": 0.5, "male": 1.0, "female": 0.0}
PRESET_KEYS = ("male", "female")
MACRO = {"age": 0.5, "muscle": 0.62, "weight": 0.45, "proportions": 0.85, "height": 0.6,
         "cupsize": 0.5, "firmness": 0.6, "race": {"asian": 0.33, "caucasian": 0.34, "african": 0.33}}
SCALE = 10.0                     # MPFB's scale factor: 0.1 gives metres, 10 centimetres
SKIN = ("skins/young_caucasian_male", "young_lightskinned_male_diffuse.png")
PROXIES = {   # slot: (MPFB asset type, .mhclo, texture)
    "eyes": ("Eyes", "eyes/low-poly/low-poly.mhclo", "eyes/materials/brown_eye.png"),
    "eyebrows": ("Eyebrows", "eyebrows/eyebrow001/eyebrow001.mhclo", "eyebrows/eyebrow001/eyebrow001.png"),
    "eyelashes": ("Eyelashes", "eyelashes/eyelashes01/eyelashes01.mhclo", "eyelashes/eyelashes01/eyelashes01.png"),
}
MASKED = ("eyebrows", "eyelashes")        # alpha-tested cards
CLOTH_TEXTURE_SIZE = 1024
SHIRT_COLOR = (0.84, 0.80, 0.72)          # undyed linen: light, so the side colour reads well
TROUSERS_COLOR = (0.42, 0.37, 0.31)
FINGER_NAMES = ("index", "middle", "ring", "pinky")


# --- MPFB ----------------------------------------------------------------------------------

def mpfb():
    """The MPFB services; a clear message if the extension or its assets are missing."""
    try:
        addon_utils.enable(MPFB, default_set=True)
        from bl_ext.user_default.mpfb.services.humanservice import HumanService
        from bl_ext.user_default.mpfb.services.locationservice import LocationService
        from bl_ext.user_default.mpfb.services.targetservice import TargetService
        from bl_ext.user_default.mpfb.services.assetservice import AssetService
    except Exception as error:   # noqa: BLE001
        raise RuntimeError("MPFB2 is not installed in Blender - run Scripts/setup_mpfb.sh (%s)" % error)
    if not AssetService.system_assets_pack_is_installed():
        raise RuntimeError("The MakeHuman system assets are missing - run Scripts/setup_mpfb.sh")
    return HumanService, LocationService, TargetService


def build_human(gender):
    """Body, rig and the face proxies of one preset. Helper geometry removed, shape keys baked."""
    HumanService, LocationService, TargetService = mpfb()
    data = LocationService.get_user_data()
    macro = dict(MACRO, gender=gender)
    body = HumanService.create_human(macro_detail_dict=macro, scale=SCALE)
    rig = HumanService.add_builtin_rig(body, "game_engine")
    proxies = {}
    for slot, (kind, path, _) in PROXIES.items():
        before = set(bpy.data.objects)
        HumanService.add_mhclo_asset(os.path.join(data, path), body, asset_type=kind, subdiv_levels=0, material_type="MAKESKIN")
        new = [o for o in bpy.data.objects if o not in before and o.type == 'MESH']
        proxies[slot] = new[0]
    TargetService.bake_targets(body)
    remove_helpers(body)
    return body, rig, proxies


def remove_helpers(body):
    """The base mesh carries helper geometry (for fitting clothes); only the group 'body' is skin."""
    for modifier in list(body.modifiers):
        if modifier.type == 'MASK':
            body.modifiers.remove(modifier)
    group = body.vertex_groups["body"].index
    keep = {v.index for v in body.data.vertices if any(g.group == group and g.weight > 0.5 for g in v.groups)}
    bm = bmesh.new()
    bm.from_mesh(body.data)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if v.index not in keep], context='VERTS')
    bm.to_mesh(body.data)
    bm.free()


def vertex_weights(obj):
    names = {g.index: g.name for g in obj.vertex_groups}
    return [{names[g.group]: g.weight for g in v.groups} for v in obj.data.vertices]


def total(weights, prefixes):
    return sum(value for name, value in weights.items() if name.startswith(prefixes))


# --- Shirt and trousers --------------------------------------------------------------------

def garment_plan(body, rig):
    """Which body vertices each garment covers and how far the cloth stands off there (cm).
    Worked out once on the neutral body; the presets reuse it (same vertex indices)."""
    bones = rig.data.bones
    hip_z = bones["thigh_l"].head_local.z
    knee_z = bones["calf_l"].head_local.z
    waist_z = hip_z + 11.0          # top of the trousers
    hem_z = hip_z + 3.5             # bottom of the shirt (above the crotch, over the trousers)
    ankle_z = bones["foot_l"].head_local.z + 5.0
    plan = {"shirt": ([], []), "trousers": ([], [])}
    for v, w in zip(body.data.vertices, vertex_weights(body)):
        co = v.co
        side = "_l" if co.x > 0 else "_r"
        shirt = trousers = False
        shirt_off = trousers_off = 0.0
        if total(w, ("upperarm", "lowerarm", "hand") + ("thumb",) + FINGER_NAMES) > 0.5:
            shoulder = bones["upperarm" + side].head_local
            a, b = bones["lowerarm" + side].head_local, bones["hand" + side].head_local
            t = (co - a).dot(b - a) / (b - a).length_squared
            if t < 0.86 and total(w, ("hand", "thumb") + FINGER_NAMES) < 0.05:
                shirt = True
                along = min(1.0, (co - shoulder).length / ((b - shoulder).length * 0.95))
                shirt_off = 1.4 + 1.3 * along * along              # sleeves widen towards the cuff
        else:
            if co.z > hem_z and total(w, ("neck", "head")) < 0.3:
                shirt = True
                shirt_off = 1.7 + 2.2 * max(0.0, min(1.0, (hip_z + 16.0 - co.z) / 12.5))   # falls loose over the hips
            if ankle_z < co.z < waist_z and total(w, ("foot", "ball")) < 0.2:
                trousers = True
                trousers_off = 1.1 + 1.6 * max(0.0, min(1.0, (knee_z - co.z) / (knee_z - ankle_z)))   # wider towards the ankle
        plan["shirt"][0].append(shirt)
        plan["shirt"][1].append(shirt_off)
        plan["trousers"][0].append(trousers)
        plan["trousers"][1].append(trousers_off)
    return plan


def taubin(bm, verts, iterations, lam=0.5, mu=-0.53):
    """Smoothing without shrinking (Taubin): removes small bumps, keeps the volume of limbs."""
    links = {v: [e.other_vert(v) for e in v.link_edges] for v in verts}
    for _ in range(iterations):
        for factor in (lam, mu):
            moved = {v: v.co + (sum((n.co for n in ns), Vector()) / len(ns) - v.co) * factor for v, ns in links.items()}
            for v, co in moved.items():
                v.co = co


def cut_garment(body, name, mask, offsets, thickness=0.3):
    """A garment object from the body: its covered faces, smoothed, never closer to the
    (slightly smoothed) skin than `offsets`, with cloth thickness. Keeps weights and UVs."""
    obj = body.copy()
    obj.data = body.data.copy()
    obj.name = obj.data.name = name
    bpy.context.scene.collection.objects.link(obj)
    for modifier in list(obj.modifiers):
        if modifier.type != 'ARMATURE':
            obj.modifiers.remove(modifier)
    obj.data.materials.clear()
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.verts.ensure_lookup_table()
    offset_of = {v: offsets[v.index] for v in bm.verts}
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if not all(mask[v.index] for v in f.verts)], context='FACES')
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
    # Hems: the cut follows the mesh grid; smooth the boundary along itself.
    rim = [v for v in bm.verts if v.is_boundary]
    for _ in range(25):
        moved = {}
        for v in rim:
            ends = [e.other_vert(v) for e in v.link_edges if e.is_boundary]
            if len(ends) == 2:
                moved[v] = v.co * 0.5 + (ends[0].co + ends[1].co) * 0.25
        for v, co in moved.items():
            v.co = co
    inner = [v for v in bm.verts if not v.is_boundary]
    original = {v: v.co.copy() for v in bm.verts}
    taubin(bm, inner, 20)                       # reference skin: big forms stay, nipples and navel go
    bm.normal_update()
    skin = {v: (v.co.copy(), v.normal.copy()) for v in bm.verts}
    for v in bm.verts:
        v.co = original[v]
    for _ in range(40):                         # the cloth bridges valleys (spine, buttocks)
        bmesh.ops.smooth_vert(bm, verts=inner, factor=0.5, use_axis_x=True, use_axis_y=True, use_axis_z=True)
    for v in bm.verts:
        co, normal = skin[v]
        v.co += normal * max(0.0, offset_of[v] - (v.co - co).dot(normal))
    bm.to_mesh(obj.data)
    bm.free()
    solid = obj.modifiers.new("Cloth thickness", 'SOLIDIFY')
    solid.thickness = thickness
    solid.offset = -1.0
    solid.use_rim = True
    solid.use_even_offset = True
    with bpy.context.temp_override(object=obj, active_object=obj):
        bpy.ops.object.modifier_move_to_index(modifier=solid.name, index=0)
        bpy.ops.object.modifier_apply(modifier=solid.name)
    return obj


# --- Assembly: presets as shape keys --------------------------------------------------------

def height(body):
    return max(v.co.z for v in body.data.vertices)


def scale_about_floor(objects, factor):
    for obj in objects:
        for v in obj.data.vertices:
            v.co *= factor


def delete_objects(objects):
    for obj in objects:
        data = obj.data
        bpy.data.objects.remove(obj, do_unlink=True)
        if data is not None and data.users == 0:
            if isinstance(data, bpy.types.Mesh):
                bpy.data.meshes.remove(data)
            elif isinstance(data, bpy.types.Armature):
                bpy.data.armatures.remove(data)


def build_model():
    """The neutral model (objects by slot and its rig) with the presets as shape keys."""
    body, rig, proxies = build_human(PRESETS["neutral"])
    plan = garment_plan(body, rig)
    parts = {"body": body}
    parts.update(proxies)
    for garment, (mask, offsets) in plan.items():
        parts[garment] = cut_garment(body, garment, mask, offsets)
    neutral_height = height(body)
    for obj in parts.values():
        obj.shape_key_add(name="Basis", from_mix=False)
    for preset in PRESET_KEYS:
        other, other_rig, other_proxies = build_human(PRESETS[preset])
        other_parts = {"body": other}
        other_parts.update(other_proxies)
        for garment, (mask, offsets) in plan.items():
            other_parts[garment] = cut_garment(other, garment + "_" + preset, mask, offsets)
        scale_about_floor(other_parts.values(), neutral_height / height(other))
        for slot, obj in parts.items():
            source = other_parts[slot]
            if len(source.data.vertices) != len(obj.data.vertices):
                raise RuntimeError("preset %s: %s has another topology" % (preset, slot))
            key = obj.shape_key_add(name=preset, from_mix=False)
            for vertex in source.data.vertices:
                key.data[vertex.index].co = vertex.co
            key.value = 0.0
        delete_objects(list(other_parts.values()) + [other_rig])
    return parts, rig


# --- Rig ------------------------------------------------------------------------------------

def hand_frame(arm, side):
    """Directions of a hand in the rest pose: along the fingers, across (pinky -> index), palm normal."""
    bones = arm.data.bones
    wrist = bones["hand" + side].head_local
    index, pinky = bones["index_01" + side].head_local, bones["pinky_01" + side].head_local
    knuckles = (index + pinky) / 2.0
    along = (knuckles - wrist).normalized()
    across = (index - pinky).normalized()
    palm = along.cross(across).normalized()
    thumb = bones["thumb_01" + side].head_local
    if (thumb - knuckles).dot(palm) < 0.0:      # the thumb sits on the palm side
        palm = -palm
    return wrist, knuckles, along, across, palm


def finish_rig(arm, meshes):
    """Root named like the mannequin, the shield's socket bone, every bone deforms, weights
    only for bones (at most 4 per vertex). The sword's socket follows once the fist is known."""
    hb.set_active(arm)
    bpy.ops.object.mode_set(mode='EDIT')
    edit = arm.data.edit_bones
    if "Root" in edit:
        edit["Root"].name = "root"
    elbow, wrist_l = edit["lowerarm_l"].head.copy(), edit["hand_l"].head.copy()
    _, _, _, _, palm_l = hand_frame(arm, "_l")
    outside = -palm_l                                          # the back of the forearm
    strap = elbow + (wrist_l - elbow) * 0.55 + outside * 4.4
    shield = edit.new("shield_l")
    shield.head, shield.tail = strap, strap + outside * 10.0   # +Y out of the shield's face
    shield.parent = edit["lowerarm_l"]
    shield.align_roll(elbow - wrist_l)
    bpy.ops.object.mode_set(mode='OBJECT')
    for bone in arm.data.bones:
        bone.use_deform = True          # also root and the sockets: the export keeps deform bones only
    names = set(arm.data.bones.keys())
    for obj in meshes:
        for group in list(obj.vertex_groups):
            if group.name not in names:
                obj.vertex_groups.remove(group)
        clean_weights(obj)
        obj.parent = arm
        obj.matrix_parent_inverse = Matrix.Identity(4)
        for modifier in obj.modifiers:
            if modifier.type == 'ARMATURE':
                modifier.object = arm
        if not any(m.type == 'ARMATURE' for m in obj.modifiers):
            obj.modifiers.new("Armature", 'ARMATURE').object = arm


def add_weapon_socket(arm):
    """weapon_r in the middle of the right fist: where the curled fingers ring round the grip
    (fists() applied; the hand itself does not turn, so the point holds in the rest pose too)."""
    apply_pose(arm, fists())
    rings = []
    for name in FINGER_NAMES:
        points = [arm.pose.bones["%s_0%d_r" % (name, n)].head for n in (1, 2, 3)] + [arm.pose.bones[name + "_03_r"].tail]
        rings.append(sum(points, Vector()) / len(points))
    apply_pose(arm, {})
    grip = sum(rings, Vector()) / len(rings)
    axis = (rings[0] - rings[-1]).normalized()                  # pinky -> index: the blade leaves above the thumb
    _, _, _, _, palm = hand_frame(arm, "_r")
    hb.set_active(arm)
    bpy.ops.object.mode_set(mode='EDIT')
    weapon = arm.data.edit_bones.new("weapon_r")
    weapon.head, weapon.tail = grip, grip + axis * 12.0         # +Y along the blade
    weapon.parent = arm.data.edit_bones["hand_r"]
    weapon.align_roll(-palm)
    bpy.ops.object.mode_set(mode='OBJECT')
    arm.data.bones["weapon_r"].use_deform = True


def clean_weights(obj, max_influences=4):
    """At most four bones per vertex, weights add up to 1 (what Unreal expects)."""
    current = [[(g.weight, g.group) for g in vertex.groups] for vertex in obj.data.vertices]
    for index, pairs in enumerate(current):
        best = sorted((pair for pair in pairs if pair[0] > 0.005), reverse=True)[:max_influences]
        sum_ = sum(weight for weight, _ in best) or 1.0
        keep = {group: weight / sum_ for weight, group in best}
        for _, group in pairs:
            if group in keep:
                obj.vertex_groups[group].add([index], keep[group], 'REPLACE')
            else:
                obj.vertex_groups[group].remove([index])


# --- Materials and textures -----------------------------------------------------------------

def mpfb_data(path):
    _, LocationService, _ = mpfb()
    return os.path.join(LocationService.get_user_data(), path)


def cloth_texture(path, color, seed):
    """Undyed cloth: a calm, uneven tone and a fine grain (UVs are the body's, no seams)."""
    n = CLOTH_TEXTURE_SIZE
    y, x = np.mgrid[0:n, 0:n].astype(np.float32) / n
    rng = np.random.default_rng(seed)
    shade = np.ones((n, n), dtype=np.float32)
    for _ in range(24):                                      # soft blotches, 5 to 60 % of the image
        angle, phase = rng.uniform(0.0, math.pi), rng.uniform(0.0, 2.0 * math.pi)
        frequency = rng.uniform(1.6, 20.0)
        shade += 0.006 * np.sin(2.0 * math.pi * frequency * (x * math.cos(angle) + y * math.sin(angle)) + phase)
    shade += rng.normal(0.0, 0.012, (n, n)).astype(np.float32)   # grain
    rgba = np.ones((n, n, 4), dtype=np.float32)
    for c in range(3):
        rgba[:, :, c] = np.clip(color[c] * shade, 0.0, 1.0)
    image = bpy.data.images.new(os.path.basename(path), n, n, alpha=True)
    image.pixels.foreach_set(rgba[::-1].ravel())
    image.filepath_raw = path
    image.file_format = 'PNG'
    image.save()
    bpy.data.images.remove(image)


def build_textures():
    """T_humanoid_<slot>.png: MakeHuman textures (CC0) copied, cloth made here."""
    os.makedirs(OUT_DIR, exist_ok=True)
    sources = {"body": os.path.join(*SKIN)}
    sources.update({slot: texture for slot, (_, _, texture) in PROXIES.items()})
    paths = {}
    for slot, source in sources.items():
        paths[slot] = os.path.join(OUT_DIR, "T_%s_%s.png" % (NAME, slot))
        shutil.copyfile(mpfb_data(source), paths[slot])
    for slot, color, seed in (("shirt", SHIRT_COLOR, 1), ("trousers", TROUSERS_COLOR, 2)):
        paths[slot] = os.path.join(OUT_DIR, "T_%s_%s.png" % (NAME, slot))
        cloth_texture(paths[slot], color, seed)
    print("build_humanoid: textures " + ", ".join(sorted(os.path.basename(p) for p in paths.values())))
    return paths


def apply_materials(parts, textures):
    """One material per slot, named like the slot (the FBX carries the names to Unreal)."""
    for slot, obj in parts.items():
        material = hb.new_material(slot)
        nodes, links = material.node_tree.nodes, material.node_tree.links
        output = next(n for n in nodes if n.type == 'OUTPUT_MATERIAL')
        image = bpy.data.images.load(textures[slot], check_existing=True)
        texture = nodes.new('ShaderNodeTexImage')
        texture.image = image
        shader = nodes.new('ShaderNodeBsdfPrincipled')
        shader.inputs['Roughness'].default_value = 0.85 if slot in ("shirt", "trousers") else 0.55
        links.new(texture.outputs['Color'], shader.inputs['Base Color'])
        if slot in MASKED:
            links.new(texture.outputs['Alpha'], shader.inputs['Alpha'])
            material.surface_render_method = 'DITHERED'
        else:
            image.alpha_mode = 'CHANNEL_PACKED'
        links.new(shader.outputs['BSDF'], output.inputs['Surface'])
        obj.data.materials.clear()
        obj.data.materials.append(material)
        if slot != "body":
            hb.shade_smooth(obj)


# --- Stand-in longsword and round shield on the sockets (in the .blend only) ----------------

def add_box(bm, low, high, taper=None):
    xs, ys, zs = zip(low, high)
    verts = [bm.verts.new((x, y, z)) for y in ys for z in zs for x in xs]   # 0..3 at the low y end
    if taper:
        for vert in verts[4:]:
            vert.co.x *= taper
            vert.co.z *= taper
    for a, b, c, d in ((0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)):
        bm.faces.new((verts[a], verts[b], verts[c], verts[d]))


def build_props(arm):
    """Local axes like the socket bones: +Y along the blade / out of the shield's face."""
    def finish(bm, name, bone, color):
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
        mesh = bpy.data.meshes.new(name)
        bm.to_mesh(mesh)
        bm.free()
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        material = hb.new_material("M_" + name)
        shader = material.node_tree.nodes.new('ShaderNodeBsdfPrincipled')
        shader.inputs['Base Color'].default_value = color
        output = next(n for n in material.node_tree.nodes if n.type == 'OUTPUT_MATERIAL')
        material.node_tree.links.new(shader.outputs['BSDF'], output.inputs['Surface'])
        mesh.materials.append(material)
        obj.parent, obj.parent_type, obj.parent_bone = arm, 'BONE', bone
        bpy.context.view_layer.update()
        obj.matrix_world = arm.matrix_world @ arm.pose.bones[bone].matrix
        return obj

    bm = bmesh.new()
    add_box(bm, (-1.7, -12.0, -1.7), (1.7, -9.0, 1.7))                      # pommel
    add_box(bm, (-1.1, -9.0, -1.4), (1.1, 9.0, 1.4))                        # grip
    add_box(bm, (-1.2, 9.0, -10.0), (1.2, 10.8, 10.0))                      # guard
    add_box(bm, (-0.4, 10.8, -2.4), (0.4, 88.0, 2.4))                       # blade
    add_box(bm, (-0.4, 88.0, -2.4), (0.4, 98.0, 2.4), taper=0.15)           # point
    sword = finish(bm, "PROP_longsword", "weapon_r", (0.55, 0.57, 0.62, 1.0))

    bm = bmesh.new()
    turn = Matrix.Rotation(math.radians(-90.0), 4, 'X')                     # cone axis Z -> Y
    bmesh.ops.create_cone(bm, cap_ends=True, segments=28, radius1=30.0, radius2=30.0, depth=2.4, matrix=Matrix.Translation((0, 1.6, 0)) @ turn)
    bmesh.ops.create_cone(bm, cap_ends=True, segments=16, radius1=7.5, radius2=4.5, depth=3.0, matrix=Matrix.Translation((0, 4.3, 0)) @ turn)
    shield = finish(bm, "PROP_round_shield", "shield_l", (0.42, 0.27, 0.15, 1.0))
    return [sword, shield]


# --- Poses ----------------------------------------------------------------------------------
# A pose is {bone: [(axis, degrees), ...]}: turns around the world axes, on top of what the
# parent bone does, in the order given.
#   "x": forward (+) / back (-): a hanging limb swings forward, the trunk and head bow forward;
#   "y": a hanging limb goes to the character's right (+) / left (-), so +y lifts the right
#        arm sideways and lowers the left one;
#   "z": turn to the character's left (+) / right (-);
#   "curl": a finger or thumb bends towards the palm (axis worked out from the rest pose).
AXES = {"x": Vector((1, 0, 0)), "y": Vector((0, 1, 0)), "z": Vector((0, 0, 1))}
UPRIGHT = {"pelvis", "spine_01", "spine_02", "spine_03", "neck_01", "neck_02", "head"}
CURL_AXES = {}          # bone -> axis in the armature's rest space
SOLE_POINTS = []        # (bone, point in the rest pose) at heels, balls and toes


def prepare_pose_data(arm, body):
    """Curl axes of the fingers and the sole points of the feet, from the rest pose."""
    CURL_AXES.clear()
    for side in ("_l", "_r"):
        _, _, along, _, palm = hand_frame(arm, side)
        for name in ("thumb",) + FINGER_NAMES:
            for n in (1, 2, 3):
                bone = arm.data.bones.get("%s_0%d%s" % (name, n, side))
                if bone is None:
                    continue
                direction = (bone.tail_local - bone.head_local).normalized()
                if name == "thumb":         # across the palm, towards the middle finger
                    towards = (arm.data.bones["middle_02" + side].head_local - bone.head_local).normalized()
                else:
                    towards = palm
                axis = direction.cross(towards)
                if axis.length > 1e-4:
                    CURL_AXES[bone.name] = axis.normalized()
    SOLE_POINTS.clear()
    weights = vertex_weights(body)
    for side in ("_l", "_r"):
        foot = [(v.co.copy(), w) for v, w in zip(body.data.vertices, weights)
                if total(w, ("foot" + side, "ball" + side)) > 0.5]
        low = min(co.z for co, _ in foot)
        sole = [(co, w) for co, w in foot if co.z < low + 1.5]
        picks = (max(sole, key=lambda p: p[0].y), min(sole, key=lambda p: p[0].y),
                 min(sole, key=lambda p: p[0].x), max(sole, key=lambda p: p[0].x))
        for co, w in picks:
            bone = "ball" + side if w.get("ball" + side, 0.0) > w.get("foot" + side, 0.0) else "foot" + side
            SOLE_POINTS.append((bone, co))


def apply_pose(arm, pose, pelvis_offset=(0.0, 0.0, 0.0), ground=False):
    """Sets every pose bone. ground=True lowers or lifts the pelvis until a foot touches the floor."""
    for pbone in arm.pose.bones:                # creation order: parents before children
        pbone.rotation_mode = 'QUATERNION'
        rest = pbone.bone.matrix_local
        inherited = pbone.parent.matrix @ pbone.parent.bone.matrix_local.inverted() @ rest if pbone.parent else rest.copy()
        location = inherited.to_translation()
        rotation = inherited.to_3x3()
        for axis, degrees in pose.get(pbone.name, ()):
            if axis == "curl":
                if pbone.name in CURL_AXES:
                    turn = (inherited.to_3x3() @ rest.to_3x3().inverted()) @ CURL_AXES[pbone.name]
                    rotation = Matrix.Rotation(math.radians(degrees), 3, turn) @ rotation
                continue
            sign = -1.0 if axis == "x" and pbone.name not in UPRIGHT else 1.0
            rotation = Matrix.Rotation(math.radians(sign * degrees), 3, AXES[axis]) @ rotation
        if pbone.name == "pelvis":
            location = location + Vector(pelvis_offset)
        pbone.matrix = Matrix.Translation(location) @ rotation.to_4x4()
        bpy.context.view_layer.update()
    if ground:
        lowest = lowest_sole(arm)
        apply_pose(arm, pose, (pelvis_offset[0], pelvis_offset[1], pelvis_offset[2] - lowest))


def lowest_sole(arm):
    """Height of the lowest sole point in the current pose (0 = on the floor)."""
    heights = []
    for bone, point in SOLE_POINTS:
        pbone = arm.pose.bones[bone]
        heights.append((arm.matrix_world @ pbone.matrix @ pbone.bone.matrix_local.inverted() @ point).z)
    return min(heights)


def fists(amount=1.0):
    """Fingers closed round a grip (both hands)."""
    pose = {}
    for suffix in ("_l", "_r"):
        for name in FINGER_NAMES:
            for n, degrees in ((1, 70), (2, 85), (3, 55)):
                pose["%s_0%d%s" % (name, n, suffix)] = [("curl", degrees * amount)]
        for n, degrees in ((1, 20), (2, 30), (3, 30)):
            pose["thumb_0%d%s" % (n, suffix)] = [("curl", degrees * amount)]
    return pose


def over(base, **bones):
    """base with the given bones replaced; the hands hold sword and shield."""
    result = fists()
    result.update(base)
    result.update(bones)
    return result


LAST_QUATERNION = {}


def key_pose(arm, frame, pose, pelvis_offset=(0.0, 0.0, 0.0), ground=True):
    apply_pose(arm, pose, pelvis_offset, ground)
    action = arm.animation_data.action
    for pbone in arm.pose.bones:
        previous = LAST_QUATERNION.get((action.name, pbone.name))
        if previous is not None and previous.dot(pbone.rotation_quaternion) < 0.0:
            pbone.rotation_quaternion.negate()          # the short way round between two keys
        LAST_QUATERNION[(action.name, pbone.name)] = pbone.rotation_quaternion.copy()
        pbone.keyframe_insert("rotation_quaternion", frame=frame)
    arm.pose.bones["pelvis"].keyframe_insert("location", frame=frame)


# The stance everything starts from: left foot and shield in front, sword hand at the hip.
GUARD = over({
    "pelvis": [("z", -18)], "spine_01": [("x", 6)], "spine_02": [("z", -6)], "neck_01": [("z", 12)], "head": [("z", 12)],
    "thigh_l": [("x", 26)], "calf_l": [("x", -32)], "foot_l": [("x", 6)],
    "thigh_r": [("x", -14)], "calf_r": [("x", -30)], "foot_r": [("x", 30)],
    "upperarm_r": [("y", -30), ("x", 8)], "lowerarm_r": [("x", 80)], "hand_r": [("x", -30)],
    "upperarm_l": [("y", 32), ("x", 30)], "lowerarm_l": [("x", 60), ("z", -35)],
})
# Test poses for the deformation renders (no actions, not exported).
DEFORM_POSES = {
    "squat": {"spine_01": [("x", 20)], "thigh_l": [("x", 95)], "calf_l": [("x", -115)], "thigh_r": [("x", 95)], "calf_r": [("x", -115)],
              "upperarm_l": [("y", -50)], "upperarm_r": [("y", -35), ("x", 90)], "lowerarm_r": [("x", 90)]},
    "reach": {"thigh_l": [("y", -45)], "thigh_r": [("x", 60)], "calf_r": [("x", -90)], "spine_02": [("z", 25)],
              "upperarm_l": [("y", -120)], "upperarm_r": [("y", -35), ("x", -40)], "lowerarm_l": [("y", -40)]},
}


def build_animations(arm):
    def start(name, last):
        action = hb.new_action(arm, name)
        for pbone in arm.pose.bones:
            pbone.rotation_mode = 'QUATERNION'
        hb.set_action_range(action, 0, last)

    start("Idle", 40)                                   # loop: the guard stance, breathing
    breathe = over(GUARD, spine_02=[("z", -6), ("x", 3)], upperarm_l=[("y", 32), ("x", 33)], upperarm_r=[("y", -30), ("x", 10)])
    key_pose(arm, 0, GUARD)
    key_pose(arm, 20, breathe, (0.0, 0.0, -0.8))
    key_pose(arm, 40, GUARD)

    start("Move", 16)                                   # loop: runs on the spot (the capsule moves), arms stay in guard

    def stride(phase):                                  # left leg in front at phase 0
        s, c = math.sin(phase * 2 * math.pi), math.cos(phase * 2 * math.pi)
        return over(GUARD, pelvis=[("z", -10 + 5 * c)], spine_01=[("x", 10)], spine_02=[("z", -6 - 8 * c)], neck_01=[("z", 8)], head=[("z", 8)],
                    thigh_l=[("x", 8 + 32 * c)], calf_l=[("x", -18 - 50 * max(0.0, -s))], foot_l=[("x", 10 - 12 * c)],
                    thigh_r=[("x", 8 - 32 * c)], calf_r=[("x", -18 - 50 * max(0.0, s))], foot_r=[("x", 10 + 12 * c)],
                    upperarm_r=[("y", -30), ("x", 8 - 10 * c)], upperarm_l=[("y", 32), ("x", 30 + 8 * c)])

    for frame in range(0, 17, 2):
        key_pose(arm, frame, stride(frame / 16.0), (0.0, 0.0, 1.5 * abs(math.sin(frame / 16.0 * 2 * math.pi))))

    # Light attack: the sword cocked over the right shoulder (wind-up), a slash across the
    # front to the left (strike), back to the guard (recovery).
    cocked = over(GUARD, spine_02=[("z", -28)], spine_03=[("z", -10)], neck_01=[("z", 24)], head=[("z", 22)],
                  upperarm_r=[("y", 15), ("x", -30)], lowerarm_r=[("x", 115)], hand_r=[("x", 25)],
                  upperarm_l=[("y", 32), ("x", 42)])
    slashed = over(GUARD, pelvis=[("z", -4)], spine_01=[("x", 12)], spine_02=[("z", 26)], spine_03=[("z", 10)], neck_01=[("z", -12)], head=[("z", -12)],
                   thigh_l=[("x", 34)], calf_l=[("x", -40)],
                   upperarm_r=[("y", -30), ("x", 72), ("z", 38)], lowerarm_r=[("x", 10), ("y", 80)], hand_r=[],
                   upperarm_l=[("y", 28), ("x", 10)], lowerarm_l=[("x", 70), ("z", -20)])
    start("Windup", 8)
    key_pose(arm, 0, GUARD)
    key_pose(arm, 8, cocked)
    start("Strike", 4)
    key_pose(arm, 0, cocked)
    key_pose(arm, 4, slashed)
    start("Recovery", 10)
    key_pose(arm, 0, slashed)
    key_pose(arm, 10, GUARD)

    # Second light attack, the other side (the game alternates the two within a chain): the
    # sword cocked across the body at the left shoulder, a backhand slash to the right.
    cocked_left = over(GUARD, spine_02=[("z", 28)], spine_03=[("z", 10)], neck_01=[("z", -22)], head=[("z", -20)],
                       upperarm_r=[("y", -30), ("x", 78), ("z", 42)], lowerarm_r=[("x", 105)], hand_r=[("x", 20)],
                       upperarm_l=[("y", 30), ("x", 20)], lowerarm_l=[("x", 70), ("z", -20)])
    slashed_right = over(GUARD, pelvis=[("z", 4)], spine_01=[("x", 12)], spine_02=[("z", -28)], spine_03=[("z", -10)], neck_01=[("z", 14)], head=[("z", 14)],
                         thigh_l=[("x", 34)], calf_l=[("x", -40)],
                         upperarm_r=[("y", -30), ("x", 64), ("z", -55)], lowerarm_r=[("x", 8)], hand_r=[],
                         upperarm_l=[("y", 30), ("x", 36)])
    start("Windup2", 8)
    key_pose(arm, 0, GUARD)
    key_pose(arm, 8, cocked_left)
    start("Strike2", 4)
    key_pose(arm, 0, cocked_left)
    key_pose(arm, 4, slashed_right)

    # Heavy attack: the sword pulled back at the hip, point at the enemy (charge, a loop while
    # the button is held), then a long thrust with a lunge.
    drawn = over(GUARD, pelvis=[("z", -30)], spine_01=[("x", 10)], spine_02=[("z", -16)], neck_01=[("z", 23)], head=[("z", 23)],
                 thigh_r=[("x", -8)], calf_r=[("x", -42)], foot_r=[("x", 36)],
                 upperarm_r=[("y", -26), ("x", -48)], lowerarm_r=[("x", 108)], hand_r=[("x", -60)],
                 upperarm_l=[("y", 28), ("x", 44)], lowerarm_l=[("x", 52), ("z", -45)])
    drawn_tense = over(drawn, spine_02=[("z", -19)], upperarm_r=[("y", -26), ("x", -52)])
    thrust = over(GUARD, pelvis=[("z", 4)], spine_01=[("x", 20)], spine_02=[("z", 22)], neck_01=[("z", -13), ("x", -8)], head=[("z", -13), ("x", -8)],
                  thigh_l=[("x", 48)], calf_l=[("x", -50)], foot_l=[("x", 2)],
                  thigh_r=[("x", -34)], calf_r=[("x", -8)], foot_r=[("x", 38)],
                  upperarm_r=[("y", -34), ("x", 62), ("z", 10)], lowerarm_r=[("x", 8)], hand_r=[("x", -70)],
                  upperarm_l=[("y", 30), ("x", -10)], lowerarm_l=[("x", 60), ("z", -20)])
    start("Charge", 12)
    key_pose(arm, 0, drawn)
    key_pose(arm, 6, drawn_tense)
    key_pose(arm, 12, drawn)
    start("HeavyStrike", 4)
    key_pose(arm, 0, drawn)
    key_pose(arm, 4, thrust)

    start("Block", 4)                                   # the shield in front of the chest, the body low behind it
    block = over(GUARD, spine_01=[("x", 12)], thigh_l=[("x", 36)], calf_l=[("x", -46)], thigh_r=[("x", -10)], calf_r=[("x", -44)], foot_r=[("x", 40)],
                 upperarm_l=[("y", 20), ("x", 52)], lowerarm_l=[("x", 42), ("z", -72)],
                 upperarm_r=[("y", -30), ("x", -12)], lowerarm_r=[("x", 70)])
    key_pose(arm, 0, GUARD)
    key_pose(arm, 4, block)

    start("Dodge", 14)                                  # a low, quick step: ducked, legs gathered
    duck = over(GUARD, spine_01=[("x", 28)], spine_02=[("z", -6), ("x", 12)], neck_01=[("z", 12), ("x", -14)], head=[("z", 12), ("x", -14)],
                thigh_l=[("x", 62)], calf_l=[("x", -84)], foot_l=[("x", 16)],
                thigh_r=[("x", 30)], calf_r=[("x", -92)], foot_r=[("x", 44)],
                upperarm_l=[("y", 32), ("x", 20)], upperarm_r=[("y", -30), ("x", -10)])
    key_pose(arm, 0, GUARD)
    key_pose(arm, 4, duck)
    key_pose(arm, 10, duck, (0.0, 0.0, 1.5))
    key_pose(arm, 14, GUARD)

    start("Jump", 12)                                   # loop while in the air: legs tucked, arms out
    air = over(GUARD, spine_01=[("x", 8)], thigh_l=[("x", 44)], calf_l=[("x", -62)], foot_l=[("x", -12)],
               thigh_r=[("x", 6)], calf_r=[("x", -48)], foot_r=[("x", -16)],
               upperarm_l=[("y", 10), ("x", 30)], upperarm_r=[("y", -8), ("x", 4)])
    air_b = over(air, thigh_l=[("x", 38)], thigh_r=[("x", 12)], upperarm_l=[("y", 6), ("x", 30)], upperarm_r=[("y", -4), ("x", 4)])
    key_pose(arm, 0, air, ground=False)
    key_pose(arm, 6, air_b, ground=False)
    key_pose(arm, 12, air, ground=False)

    start("Death", 16)                                  # falls on its back and stays there (the last frame is held)
    falling = dict(fists(0.6), **{"pelvis": [("x", -38)], "spine_01": [("x", -8)], "head": [("x", 14)],
                                  "thigh_l": [("x", 40)], "calf_l": [("x", -60)], "thigh_r": [("x", 16)], "calf_r": [("x", -36)],
                                  "upperarm_l": [("y", -30), ("x", 30)], "upperarm_r": [("y", 30), ("x", 30)], "lowerarm_l": [("x", 30)], "lowerarm_r": [("x", 30)]})
    lying = dict(fists(0.35), **{"pelvis": [("x", -88)], "head": [("x", 6)],
                                 "thigh_l": [("x", 14), ("z", 8)], "calf_l": [("x", -22)], "thigh_r": [("z", -8)], "calf_r": [("x", -4)],
                                 "upperarm_l": [("y", -25), ("x", -6)], "upperarm_r": [("y", 35), ("x", -6)], "lowerarm_l": [("y", -30)], "lowerarm_r": [("y", 20)]})
    key_pose(arm, 0, GUARD)
    key_pose(arm, 7, falling, (0.0, 14.0, -34.0), ground=False)
    key_pose(arm, 13, lying, (0.0, 30.0, -86.0), ground=False)
    key_pose(arm, 16, lying, (0.0, 30.0, -87.0), ground=False)

    arm.animation_data.action = None
    apply_pose(arm, {})


# --- Report and previews --------------------------------------------------------------------

def report(parts, arm):
    vertices = sum(len(o.data.vertices) for o in parts.values())
    triangles = 0
    for obj in parts.values():
        obj.data.calc_loop_triangles()
        triangles += len(obj.data.loop_triangles)
    influences = max(len(v.groups) for o in parts.values() for v in o.data.vertices)
    body = parts["body"]
    size = body.dimensions
    print("build_humanoid: size %.0f x %.0f x %.1f cm, %d vertices, %d triangles in the game, at most %d bones per vertex, slots: %s"
          % (size.x, size.y, size.z, vertices, triangles, influences,
             ", ".join("%s (%d)" % (slot, len(o.data.vertices)) for slot, o in parts.items())))
    print("build_humanoid: %d bones (%s), shape keys: %s, actions: %s"
          % (len(arm.data.bones), ", ".join(b.name for b in arm.data.bones), ", ".join(k.name for k in body.data.shape_keys.key_blocks[1:]),
             ", ".join(sorted(a.name for a in bpy.data.actions))))


def scale_legs(arm, factor):
    """Bone-length test: thigh and calf scaled along the bone, children keep their own scale;
    the armature is moved so the feet stay on the floor."""
    for suffix in ("_l", "_r"):
        for bone in ("calf", "foot"):
            arm.data.bones[bone + suffix].inherit_scale = 'FULL' if factor == 1.0 else 'NONE'
        for bone in ("thigh", "calf"):
            arm.pose.bones[bone + suffix].scale = (1.0, factor, 1.0)
    arm.location.z = 0.0
    bpy.context.view_layer.update()
    if factor != 1.0:
        arm.location.z = -lowest_sole(arm)
        bpy.context.view_layer.update()


def preview(parts, arm, props):
    def set_preset(preset):
        for obj in parts.values():
            for key in obj.data.shape_keys.key_blocks[1:]:
                key.value = 1.0 if key.name == preset else 0.0

    def show(props_visible, clothes_visible=True):
        for prop in props:
            prop.hide_render = not props_visible
        for slot in ("shirt", "trousers"):
            parts[slot].hide_render = not clothes_visible

    turnaround = (("front", 0.0), ("front_left", -35.0), ("side", -90.0), ("back", 180.0))
    two = (("front_left", -35.0), ("side", -90.0))
    arm.animation_data.action = None
    head = arm.data.bones["head"].head_local

    # The three presets in the rest pose, with and without clothes; faces close up.
    show(False)
    apply_pose(arm, {})
    for preset in ("neutral",) + PRESET_KEYS:
        set_preset(preset)
        hb.render_preview("%s_rest_%s" % (NAME, preset), (0, 0, 92), 330, size=(600, 800), views=turnaround)
        hb.render_preview("%s_face_%s" % (NAME, preset), (head.x, head.y - 2.0, head.z + 6.0), 55, size=(600, 600),
                          views=(("front", 0.0), ("front_left", -35.0), ("side", -90.0)))
    set_preset("neutral")
    show(False, clothes_visible=False)
    hb.render_preview("%s_rest_body" % NAME, (0, 0, 92), 330, size=(600, 800), views=(("front_left", -35.0), ("back", 180.0)))
    show(False)
    hand = arm.data.bones["hand_l"].head_local + (arm.data.bones["middle_01_l"].head_local - arm.data.bones["hand_l"].head_local) * 1.3
    hb.render_preview("%s_hand_open" % NAME, tuple(hand), 48, size=(600, 600), views=(("front_left", -35.0), ("side", -90.0)))
    apply_pose(arm, fists())
    hb.render_preview("%s_hand_fist" % NAME, tuple(hand), 48, size=(600, 600), views=(("front_left", -35.0), ("side", -90.0)))
    show(True)
    hand_r = Vector((-hand.x, hand.y, hand.z))
    hb.render_preview("%s_hand_grip" % NAME, tuple(hand_r), 60, size=(600, 600), views=(("front_right", 35.0), ("side_right", 90.0)))
    show(False)

    # Deformation: shoulders, hips and knees bent far, also with the legs at 80 % length.
    for pose_name, pose in DEFORM_POSES.items():
        for preset in ("neutral",) + PRESET_KEYS:
            set_preset(preset)
            apply_pose(arm, pose, ground=True)
            views = (("front_left", -35.0), ("side", -90.0), ("back_right", 145.0)) if preset == "neutral" else (("front_left", -35.0),)
            hb.render_preview("%s_deform_%s_%s" % (NAME, pose_name, preset), (0, 0, 80), 330, size=(600, 700), views=views)
        set_preset("neutral")
        scale_legs(arm, 0.8)
        hb.render_preview("%s_deform_%s_legs80" % (NAME, pose_name), (0, 0, 80), 330, size=(600, 700), views=two)
        scale_legs(arm, 1.0)
    apply_pose(arm, {})
    scale_legs(arm, 0.8)
    hb.render_preview("%s_rest_legs80" % NAME, (0, 0, 92), 330, size=(600, 800), views=(("front", 0.0), ("side", -90.0)))
    scale_legs(arm, 1.0)
    apply_pose(arm, {})

    # Key poses of the placeholder animations, with sword and shield.
    show(True)
    shots = (("Idle", 0), ("Move", 0), ("Move", 4), ("Windup", 8), ("Strike", 4), ("Windup2", 8), ("Strike2", 4), ("Charge", 0),
             ("HeavyStrike", 4), ("Recovery", 5), ("Block", 4), ("Dodge", 5), ("Jump", 0), ("Death", 7), ("Death", 16))
    for action_name, frame in shots:
        arm.animation_data.action = bpy.data.actions[action_name]
        bpy.context.scene.frame_set(frame)
        hb.render_preview("%s_%s_%d" % (NAME, action_name, frame), (0, 0, 85), 380, size=(600, 700), views=two)
    for action_name, frame in (("Idle", 0), ("Strike", 4), ("Block", 4)):
        arm.animation_data.action = bpy.data.actions[action_name]
        bpy.context.scene.frame_set(frame)
        scale_legs(arm, 0.8)
        hb.render_preview("%s_%s_%d_legs80" % (NAME, action_name, frame), (0, 0, 85), 380, size=(600, 700), views=(("front_left", -35.0),))
        scale_legs(arm, 1.0)
    arm.animation_data.action = None
    apply_pose(arm, {})


def main():
    hb.reset_scene()
    parts, arm = build_model()
    finish_rig(arm, list(parts.values()))
    textures = build_textures()
    apply_materials(parts, textures)
    prepare_pose_data(arm, parts["body"])
    add_weapon_socket(arm)
    props = build_props(arm)
    build_animations(arm)
    report(parts, arm)

    hb.save_blend(os.path.join(NAME, NAME + ".blend"))
    hb.export_model(NAME, arm, list(parts.values()))
    hb.export_actions(NAME, arm)
    if "nopreview" not in hb.script_args():
        preview(parts, arm, props)
    print("build_humanoid: done")


main()
