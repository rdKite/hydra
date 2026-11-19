"""Builds the humanoid base model: one skeleton for all humanoids, a plain gender-neutral
stand-in body, the three body presets as shape keys, hand sockets, placeholder animations,
FBX export.

  blender -b --python Art/scripts/build_humanoid.py            (add "-- nopreview" to skip the renders)

What it is (roadmap M5, first item; decisions of 2026-11-19: one shared humanoid base model
for all races, gender-fluid sliders):
- Body: a quad cage written out ring by ring (torso lattice, 8-sided limbs, extra loops at
  shoulder, elbow, wrist, hip, knee and ankle), mirrored, then one Catmull-Clark level. Not a
  final character: no face, a mitten hand, no fingers.
- Skin weights are set per cage ring and smoothed by the subdivision, so every loop knows
  its bones (no weights by distance: the legs and the arm next to the trunk would bleed).
- Skeleton: bone names of the Unreal mannequin (root, pelvis, spine_01..03, neck_01, head,
  clavicle / upperarm / lowerarm / hand, thigh / calf / foot / ball, _l and _r), A-pose.
  Bone axes are Blender's (Y along the bone), not the mannequin's. Two socket bones:
  weapon_r (child of hand_r, +Y along the blade) and shield_l (child of lowerarm_l, +Y out
  of the shield's face).
- Body presets: the cage is built three times with other measures; "male" and "female" are
  shape keys (morph targets in Unreal), the neutral preset is the base shape. The skeleton
  is the same for all three.
- Placeholder animations, one action per clip the game looks up (A_humanoid_<Name>): Idle,
  Move, Windup, Charge, Strike, HeavyStrike, Recovery, Block, Dodge, Jump, Death. The
  human's animations replace them by name (docs/tech/blender-pipeline.md).

Writes Art/humanoid/humanoid.blend (with a stand-in longsword and round shield on the
sockets, for animating; they are not exported), T_humanoid_color.png,
Art/export/humanoid/*.fbx and preview renders Saved/ArtPreview/humanoid_*.png.
"""
import math
import os
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import hydra_blender as hb  # noqa: E402

NAME = "humanoid"
SUBDIVISION = 1
PRESET_KEYS = ("male", "female")     # the neutral preset is the base shape

# The limit surface of a subdivided cage lies inside the cage; these factors make up for it.
K_RING = 1.0 / 0.90                  # 8-sided rings
K_TORSO = 1.06

# --- Joints (centimetres, facing -Y, the character's left is +X; only the left side) --------
ARM_DIR = Vector((math.sin(math.radians(40.0)), 0.0, -math.cos(math.radians(40.0))))   # A-pose
FOREARM_DIR = Vector((0.62, -0.10, -0.78)).normalized()
SHOULDER = Vector((20.0, 0.0, 145.5))
ELBOW = SHOULDER + ARM_DIR * 28.0
WRIST = ELBOW + FOREARM_DIR * 25.0
HIP = Vector((10.5, 0.0, 96.0))
KNEE = Vector((11.0, -0.6, 52.0))
ANKLE = Vector((11.0, 1.5, 9.0))
BALL = Vector((11.4, -14.0, 2.4))
TOE = Vector((11.4, -23.0, 2.4))
# Points under the foot that may touch the floor: (bone, rest position).
SOLE_POINTS = (("foot", Vector((11.0, 6.0, 0.0))), ("foot", Vector((11.4, -14.0, 0.0))), ("ball", Vector((11.4, -23.0, 0.0))))

# --- Measures of the body -------------------------------------------------------------------
# Torso layers from the groin up: z, half width, depth to the front, depth to the back,
# centre y, extra height of the three columns (centre line, half way, side).
TORSO = [
    (88.0, 18.0, 9.8, 10.8, 0.5, (-3.0, 0.0, 6.0)),    # 0 groin: the leg rings start here
    (99.0, 18.4, 10.6, 12.0, 0.8, (0.0, 0.0, 0.0)),    # 1 hips
    (110.0, 15.4, 9.8, 9.6, 0.3, (0.0, 0.0, 0.0)),     # 2 waist
    (122.0, 16.6, 11.2, 10.6, 0.0, (0.0, 0.0, 0.0)),   # 3 ribs
    (133.0, 18.0, 12.0, 11.2, 0.0, (0.0, 0.0, 0.0)),   # 4 chest, lower edge of the arm hole
    (141.5, 18.6, 11.0, 10.4, 0.0, (0.0, 0.0, 0.0)),   # 5 middle of the arm hole
    (148.5, 17.5, 8.6, 8.6, 0.5, (1.6, 1.2, 0.0)),     # 6 shoulder line, base of the neck
]
# Neck and head rings: z, half width, depth front, depth back, centre y.
HEAD = [
    (154.0, 4.9, 4.9, 5.1, 0.3),      # neck
    (158.0, 6.2, 8.0, 6.2, 0.0),      # jaw line
    (163.5, 8.1, 10.2, 9.2, 0.0),
    (170.5, 8.6, 10.3, 10.2, 0.2),
    (177.0, 6.8, 7.6, 8.2, 0.4),
]
HEAD_TOP = Vector((0.0, 0.3, 181.6))
# Arm rings: (distance from the joint, thickness across the arm plane, thickness front to back).
UPPER_ARM = [(4.0, 6.6, 6.6), (12.0, 5.8, 5.8), (21.0, 5.1, 5.1), (24.5, 4.8, 4.8), (28.0, 4.7, 4.7)]
FOREARM = [(3.5, 4.8, 4.8), (10.0, 4.9, 4.9), (19.0, 3.9, 3.9), (22.5, 3.2, 3.6), (25.0, 2.8, 3.5)]
HAND = [(4.0, 2.3, 4.6), (10.0, 2.2, 5.0), (15.5, 1.8, 4.0)]
HAND_LENGTH = 18.5
# Leg rings: z, centre x, centre y, radius sideways, radius front to back.
LEG = [
    (79.0, 10.6, 0.3, 8.8, 9.6),      # upper thigh
    (68.0, 10.8, 0.0, 8.0, 8.6),
    (58.5, 11.0, -0.3, 6.8, 7.2),
    (55.5, 11.0, -0.5, 6.4, 6.8),
    (52.0, 11.0, -0.6, 6.1, 6.7),     # knee
    (48.5, 11.0, -0.3, 5.9, 6.4),
    (44.0, 11.0, 0.2, 6.0, 6.6),
    (35.0, 11.0, 1.2, 6.2, 7.0),      # calf
    (20.0, 11.0, 1.4, 4.4, 4.9),
    (12.5, 11.0, 1.5, 3.8, 4.3),
    (9.0, 11.0, 1.5, 3.8, 4.5),       # ankle
    (0.0, 11.0, 1.8, 4.2, 5.4),       # sole under the heel
]
# Foot rings in front of the ankle: y, height, half width, centre x.
FOOT = [(-9.0, 7.0, 4.6, 11.2), (-14.0, 5.0, 5.2, 11.4), (-19.5, 3.9, 4.8, 11.4), (-23.5, 3.0, 3.6, 11.4)]

# Skin weights per cage ring (left side; mirrored for the right).
TORSO_WEIGHTS = [   # per layer: centre line, half way, side
    ({"pelvis": 1.0}, {"pelvis": 0.6, "thigh_l": 0.4}, {"pelvis": 0.45, "thigh_l": 0.55}),
    ({"pelvis": 1.0}, {"pelvis": 1.0}, {"pelvis": 0.85, "thigh_l": 0.15}),
    ({"pelvis": 0.4, "spine_01": 0.6},) * 3,
    ({"spine_01": 0.4, "spine_02": 0.6},) * 3,
    ({"spine_02": 0.5, "spine_03": 0.5}, {"spine_02": 0.5, "spine_03": 0.5}, {"spine_02": 0.35, "spine_03": 0.5, "upperarm_l": 0.15}),
    ({"spine_03": 1.0}, {"spine_03": 0.8, "clavicle_l": 0.2}, {"spine_03": 0.5, "clavicle_l": 0.25, "upperarm_l": 0.25}),
    ({"spine_03": 0.7, "neck_01": 0.3}, {"spine_03": 0.55, "neck_01": 0.15, "clavicle_l": 0.3}, {"clavicle_l": 0.55, "spine_03": 0.2, "upperarm_l": 0.25}),
]
HEAD_WEIGHTS = [{"neck_01": 1.0}, {"neck_01": 0.5, "head": 0.5}, {"head": 1.0}, {"head": 1.0}, {"head": 1.0}]
UPPER_ARM_WEIGHTS = [{"upperarm_l": 0.75, "clavicle_l": 0.25}, {"upperarm_l": 1.0}, {"upperarm_l": 0.9, "lowerarm_l": 0.1},
                     {"upperarm_l": 0.75, "lowerarm_l": 0.25}, {"upperarm_l": 0.5, "lowerarm_l": 0.5}]
FOREARM_WEIGHTS = [{"upperarm_l": 0.25, "lowerarm_l": 0.75}, {"lowerarm_l": 1.0}, {"lowerarm_l": 0.9, "hand_l": 0.1},
                   {"lowerarm_l": 0.75, "hand_l": 0.25}, {"lowerarm_l": 0.5, "hand_l": 0.5}]
LEG_WEIGHTS = [{"thigh_l": 0.8, "pelvis": 0.2}, {"thigh_l": 1.0}, {"thigh_l": 0.9, "calf_l": 0.1}, {"thigh_l": 0.75, "calf_l": 0.25},
               {"thigh_l": 0.5, "calf_l": 0.5}, {"thigh_l": 0.25, "calf_l": 0.75}, {"thigh_l": 0.1, "calf_l": 0.9}, {"calf_l": 1.0},
               {"calf_l": 1.0}, {"calf_l": 0.8, "foot_l": 0.2}, {"calf_l": 0.5, "foot_l": 0.5}, {"foot_l": 1.0}]
FOOT_WEIGHTS = [{"foot_l": 1.0}, {"foot_l": 0.5, "ball_l": 0.5}, {"ball_l": 1.0}, {"ball_l": 1.0}]

# Body presets: what differs from the neutral measures. torso: per layer (half width, depth
# front, depth back) in centimetres; the other numbers are factors on the ring radii.
# Joints do not move, so one skeleton and one set of animations fits all presets.
PRESETS = {
    "neutral": {},
    "male": {
        "torso": {1: (-0.8, 0.0, -0.8), 2: (2.0, 0.7, 0.6), 3: (2.0, 0.9, 0.8), 4: (2.4, 1.0, 1.0), 5: (2.2, 0.7, 0.8), 6: (1.4, 0.5, 0.7)},
        "deltoid": 1.16, "upper_arm": 1.12, "forearm": 1.10, "hand": 1.08,
        "thigh_top": 1.0, "thigh": 1.05, "calf": 1.07, "foot": 1.06, "neck": 1.15, "jaw": 1.08,
    },
    "female": {
        "torso": {0: (1.0, 0.0, 0.8), 1: (2.0, 0.2, 1.6), 2: (-2.4, -0.9, -0.6), 3: (-1.6, 0.0, -0.5), 4: (-1.3, 0.0, -0.5), 5: (-1.3, -0.2, -0.4), 6: (-1.0, 0.0, 0.0)},
        "bust": {3: 2.0, 4: 3.4, 5: 0.8},
        "deltoid": 0.93, "upper_arm": 0.91, "forearm": 0.91, "hand": 0.93,
        "thigh_top": 1.07, "thigh": 1.02, "calf": 0.95, "foot": 0.94, "neck": 0.9, "jaw": 0.93,
    },
}


# --- Cage -----------------------------------------------------------------------------------

class Cage:
    """Left half of the body as quads; mirror() adds the right half."""

    def __init__(self):
        self.verts, self.weights, self.faces = [], [], []

    def vert(self, co, weights):
        self.verts.append(Vector(co))
        self.weights.append(dict(weights))
        return len(self.verts) - 1

    def quad(self, a, b, c, d):
        self.faces.append((a, b, c, d))

    def bridge(self, lower, upper, closed=True):
        count = len(lower)
        for m in range(count if closed else count - 1):
            n = (m + 1) % count
            self.quad(lower[m], lower[n], upper[n], upper[m])

    def ring(self, centre, u, v, radius_u, radius_v, weights, count=8, start_deg=-45.0):
        """Points around centre; number m sits at start + m * 360 / count degrees from u towards v."""
        result = []
        for m in range(count):
            angle = math.radians(start_deg + m * 360.0 / count)
            result.append(self.vert(centre + (u * (math.cos(angle) * radius_u) + v * (math.sin(angle) * radius_v)) * K_RING, weights))
        return result

    def cap(self, ring, centre, weights):
        middle = self.vert(centre, weights)
        for m in range(0, len(ring), 2):
            self.quad(ring[m - 1], ring[m], ring[m + 1], middle)

    def mirror(self):
        mirrored = {}
        for index in range(len(self.verts)):
            co = self.verts[index]
            if abs(co.x) < 1e-4:
                co.x = 0.0
                mirrored[index] = index
            else:
                swapped = {(b[:-2] + "_r" if b.endswith("_l") else b): w for b, w in self.weights[index].items()}
                mirrored[index] = self.vert((-co.x, co.y, co.z), swapped)
        self.faces += [tuple(mirrored[i] for i in reversed(face)) for face in self.faces]


def arm_frame(direction):
    """Axes of an arm ring: along the arm, across it in the arm plane (up and out), to the back."""
    d = direction.normalized()
    u = Vector((-d.z, 0.0, d.x)).normalized()
    return d, u, u.cross(d)


def torso_weights(k, a, j):
    """Weights of a torso point. The buttocks (back row of groin and hips) follow the thigh
    less: with the full share a deep squat folds them into a sharp ridge."""
    weights = dict(TORSO_WEIGHTS[k][a])
    if j == 2 and "thigh_l" in weights:
        moved = weights["thigh_l"] * 0.6
        weights["thigh_l"] -= moved
        weights["pelvis"] = weights.get("pelvis", 0.0) + moved
    return weights


def build_cage(preset):
    spec = PRESETS[preset]
    factor = lambda key: spec.get(key, 1.0)   # noqa: E731
    cage = Cage()

    # Torso: a lattice of 3 columns (centre line, half way, side) x 3 rows (front, middle, back)
    # per layer; only the points on the surface exist.
    def torso_point(k, a, j):
        z, half_width, front, back, centre_y, rise = TORSO[k]
        d_width, d_front, d_back = spec.get("torso", {}).get(k, (0.0, 0.0, 0.0))
        half_width, front, back = half_width + d_width, front + d_front, back + d_back
        if j == 1:
            x, y = half_width * (0.0, 0.55, 1.0)[a], 0.0
        else:
            x = half_width * (0.0, 0.55, 0.86)[a]
            y = (-front if j == 0 else back) * (1.0, 0.93, 0.58)[a]
            if j == 0:
                y -= spec.get("bust", {}).get(k, 0.0) * (0.45, 1.0, 0.0)[a]
        return Vector((x * K_TORSO, centre_y + y * K_TORSO, z + rise[a]))

    half_ring = [(0, 0), (1, 0), (2, 0), (2, 1), (2, 2), (1, 2), (0, 2)]
    top = len(TORSO) - 1
    torso = []
    for k in range(len(TORSO)):
        layer = {}
        for a, j in half_ring:
            if (a, j) == (2, 1) and k == top - 1:
                continue                                    # the middle of the arm hole
            layer[(a, j)] = cage.vert(torso_point(k, a, j), torso_weights(k, a, j))
        torso.append(layer)
    for k in range(top):
        for s in range(len(half_ring) - 1):
            p, q = half_ring[s], half_ring[s + 1]
            if k >= top - 2 and p[0] == 2 and q[0] == 2:
                continue                                    # arm hole
            cage.quad(torso[k][p], torso[k][q], torso[k + 1][q], torso[k + 1][p])
    shoulder_top = cage.vert(torso_point(top, 1, 1), TORSO_WEIGHTS[top][1])
    cage.quad(torso[top][(1, 0)], torso[top][(2, 0)], torso[top][(2, 1)], shoulder_top)
    cage.quad(shoulder_top, torso[top][(2, 1)], torso[top][(2, 2)], torso[top][(1, 2)])

    # Neck and head: half rings from the front over the left side to the back.
    below = [torso[top][(0, 0)], torso[top][(1, 0)], shoulder_top, torso[top][(1, 2)], torso[top][(0, 2)]]
    for index, (z, half_width, front, back, centre_y) in enumerate(HEAD):
        scale = factor("neck") if index == 0 else factor("jaw") if index == 1 else (1.0 + factor("jaw")) * 0.5 if index == 2 else 1.0
        ring = []
        for m in range(5):
            angle = math.radians(45.0 * m)
            depth = front if m < 2 else back
            ring.append(cage.vert((half_width * scale * math.sin(angle) * K_RING, centre_y - depth * math.cos(angle) * K_RING, z), HEAD_WEIGHTS[index]))
        cage.bridge(below, ring, closed=False)
        below = ring
    crown = cage.vert(HEAD_TOP, {"head": 1.0})
    cage.quad(below[0], below[1], below[2], crown)
    cage.quad(below[2], below[3], below[4], crown)

    # Arm: out of the hole in the side of the chest. Ring points: top front, top, top back,
    # back, bottom back, bottom, bottom front, front.
    t4, t5, t6 = torso[top - 2], torso[top - 1], torso[top]
    below = [t6[(2, 0)], t6[(2, 1)], t6[(2, 2)], t5[(2, 2)], t4[(2, 2)], t4[(2, 1)], t4[(2, 0)], t5[(2, 0)]]
    d, u, v = arm_frame(ARM_DIR)
    for index, (distance, across, depth) in enumerate(UPPER_ARM):
        scale = factor("deltoid") if index == 0 else factor("upper_arm") if index < 3 else (1.0 + factor("upper_arm")) * 0.5
        _, ring_u, ring_v = arm_frame(ARM_DIR + Vector((1.0, 0.0, 0.0))) if index == 0 else (d, u, v)
        ring = cage.ring(SHOULDER + d * distance, ring_u, ring_v, across * scale, depth * scale, UPPER_ARM_WEIGHTS[index])
        cage.bridge(below, ring)
        below = ring
    d, u, v = arm_frame(FOREARM_DIR)
    for index, (distance, across, depth) in enumerate(FOREARM):
        scale = factor("forearm") if index in (1, 2) else (1.0 + factor("forearm")) * 0.5
        ring = cage.ring(ELBOW + d * distance, u, v, across * scale, depth * scale, FOREARM_WEIGHTS[index])
        cage.bridge(below, ring)
        below = ring
    for distance, across, depth in HAND:
        ring = cage.ring(WRIST + d * distance, u, v, across * factor("hand"), depth * factor("hand"), {"hand_l": 1.0})
        cage.bridge(below, ring)
        below = ring
    cage.cap(below, WRIST + d * HAND_LENGTH, {"hand_l": 1.0})

    # Leg: out of the groin layer. Ring points: front inner, front, front outer, outer, back
    # outer, back, back inner, inner.
    groin = torso[0]
    crotch = cage.vert((0.0, TORSO[0][4], TORSO[0][0] - 4.0), {"pelvis": 1.0})
    below = [groin[(0, 0)], groin[(1, 0)], groin[(2, 0)], groin[(2, 1)], groin[(2, 2)], groin[(1, 2)], groin[(0, 2)], crotch]
    forward, outward = Vector((0.0, -1.0, 0.0)), Vector((1.0, 0.0, 0.0))
    rings = []
    for index, (z, centre_x, centre_y, sideways, depth) in enumerate(LEG):
        scale = factor("thigh_top") if index == 0 else factor("thigh") if index < 3 else factor("calf") if 6 <= index <= 8 else 1.0
        if index == len(LEG) - 1:
            scale = factor("foot")
        ring = cage.ring(Vector((centre_x, centre_y, z)), forward, outward, depth * scale, sideways * scale, LEG_WEIGHTS[index])
        rings.append(ring)
        if index < len(LEG) - 1:
            cage.bridge(below, ring)
        below = ring
    ankle, sole = rings[-2], rings[-1]
    for m in range(2, 8):                                   # the two quads at the front stay open for the foot
        cage.quad(ankle[m], ankle[(m + 1) % 8], sole[(m + 1) % 8], sole[m])
    cage.cap(sole, Vector((LEG[-1][1], LEG[-1][2], 0.0)), {"foot_l": 1.0})

    # Foot: 6-sided rings (top inner, top, top outer, bottom outer, bottom, bottom inner).
    below = [ankle[0], ankle[1], ankle[2], sole[2], sole[1], sole[0]]
    for index, (y, height, half_width, centre_x) in enumerate(FOOT):
        w = half_width * factor("foot") * K_RING
        low = 0.0 if index < len(FOOT) - 1 else 0.7
        ring = [cage.vert(co, FOOT_WEIGHTS[index]) for co in (
            (centre_x - w * 0.8, y, height * 0.85), (centre_x, y, height * 1.05), (centre_x + w * 0.8, y, height * 0.85),
            (centre_x + w, y, low), (centre_x, y, low), (centre_x - w, y, low))]
        cage.bridge(below, ring)
        below = ring
    cage.quad(below[0], below[1], below[4], below[5])
    cage.quad(below[1], below[2], below[3], below[4])

    cage.mirror()
    return cage


def build_body(preset, name):
    """The cage of a preset as a subdivided mesh object with vertex groups (weights)."""
    cage = build_cage(preset)
    obj = hb.new_mesh_object(name, [tuple(v) for v in cage.verts], cage.faces)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(obj.data)
    bm.free()
    groups = {}
    for index, weights in enumerate(cage.weights):
        for bone, weight in weights.items():
            if bone not in groups:
                groups[bone] = obj.vertex_groups.new(name=bone)
            groups[bone].add([index], weight, 'REPLACE')
    hb.set_active(obj)
    modifier = obj.modifiers.new("Subdivision", 'SUBSURF')
    modifier.levels = SUBDIVISION
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    for vertex in obj.data.vertices:
        vertex.co.z = max(vertex.co.z, 0.0)
    return obj


def clean_weights(obj, max_influences=4):
    """At most four bones per vertex, weights add up to 1 (what Unreal expects)."""
    current = [[(g.weight, g.group) for g in vertex.groups] for vertex in obj.data.vertices]   # read first, then change
    for index, pairs in enumerate(current):
        best = sorted((pair for pair in pairs if pair[0] > 0.005), reverse=True)[:max_influences]
        total = sum(weight for weight, _ in best)
        keep = {group: weight / total for weight, group in best}
        for _, group in pairs:
            if group in keep:
                obj.vertex_groups[group].add([index], keep[group], 'REPLACE')
            else:
                obj.vertex_groups[group].remove([index])


def build_mesh():
    obj = build_body("neutral", "SK_" + NAME)
    obj.data.name = "SK_" + NAME
    clean_weights(obj)
    hb.shade_smooth(obj)
    hb.set_active(obj)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.02)
    bpy.ops.object.mode_set(mode='OBJECT')
    return obj


def build_shape_keys(obj):
    """The presets have the same cage topology, so their vertices are the shape keys."""
    obj.shape_key_add(name="Basis", from_mix=False)
    for preset in PRESET_KEYS:
        variant = build_body(preset, "preset_" + preset)
        if len(variant.data.vertices) != len(obj.data.vertices):
            raise RuntimeError("preset %s has another topology than the neutral body" % preset)
        key = obj.shape_key_add(name=preset, from_mix=False)
        for vertex in variant.data.vertices:
            key.data[vertex.index].co = vertex.co
        key.value = 0.0
        mesh = variant.data
        bpy.data.objects.remove(variant, do_unlink=True)
        bpy.data.meshes.remove(mesh)


# --- Rig ------------------------------------------------------------------------------------

def side_vector(v, sign):
    return Vector((v.x * sign, v.y, v.z))


def socket_bones():
    """Where the hands hold things, right hand and left forearm: name -> (parent, head, tail, roll target)."""
    d, u, _ = arm_frame(FOREARM_DIR)
    # Sword grip: in the palm (the side of the hand that faces the body), blade to the front.
    grip = WRIST + d * 8.0 - u * 2.6
    blade = (Vector((0.0, -1.0, 0.0)) - d * d.dot(Vector((0.0, -1.0, 0.0)))).normalized()
    grip_r, blade_r = side_vector(grip, -1.0), side_vector(blade, -1.0)
    # Shield: on the back of the forearm, its face looks away from the arm.
    strap = ELBOW + d * 13.0 + u * 4.2
    return {
        "weapon_r": ("hand_r", grip_r, grip_r + blade_r * 12.0, Vector((0.0, 0.0, 1.0))),
        "shield_l": ("lowerarm_l", strap, strap + u * 10.0, -d),
    }


def bone_list():
    front, up = Vector((0.0, -1.0, 0.0)), Vector((0.0, 0.0, 1.0))
    bones = [   # name, parent, head, tail, direction the bone's Z axis looks to (roll)
        ("root", None, Vector((0, 0, 0)), Vector((0, 8, 0)), up),
        ("pelvis", "root", Vector((0, 0.5, 98)), Vector((0, 0.5, 108)), front),
        ("spine_01", "pelvis", Vector((0, 0.5, 108)), Vector((0, 0, 121)), front),
        ("spine_02", "spine_01", Vector((0, 0, 121)), Vector((0, 0, 134)), front),
        ("spine_03", "spine_02", Vector((0, 0, 134)), Vector((0, 0.5, 151)), front),
        ("neck_01", "spine_03", Vector((0, 0.5, 151)), Vector((0, 0, 158.5)), front),
        ("head", "neck_01", Vector((0, 0, 158.5)), Vector((0, 0, 180)), front),
    ]
    for suffix, sign in (("_l", 1.0), ("_r", -1.0)):
        p = lambda v: side_vector(v, sign)   # noqa: E731
        bones += [
            ("clavicle" + suffix, "spine_03", p(Vector((2.5, 0, 147))), p(SHOULDER), front),
            ("upperarm" + suffix, "clavicle" + suffix, p(SHOULDER), p(ELBOW), front),
            ("lowerarm" + suffix, "upperarm" + suffix, p(ELBOW), p(WRIST), front),
            ("hand" + suffix, "lowerarm" + suffix, p(WRIST), p(WRIST + FOREARM_DIR * 9.0), front),
            ("thigh" + suffix, "pelvis", p(HIP), p(KNEE), front),
            ("calf" + suffix, "thigh" + suffix, p(KNEE), p(ANKLE), front),
            ("foot" + suffix, "calf" + suffix, p(ANKLE), p(BALL), up),
            ("ball" + suffix, "foot" + suffix, p(BALL), p(TOE), up),
        ]
    for name, (parent, head, tail, roll) in socket_bones().items():
        bones.append((name, parent, head, tail, roll))
    return bones


def build_rig(obj):
    bones = bone_list()
    arm = hb.build_armature("Armature", [(n, p, tuple(h), tuple(t)) for n, p, h, t, _ in bones])
    hb.set_active(arm)
    bpy.ops.object.mode_set(mode='EDIT')
    for name, _, _, _, roll in bones:
        arm.data.edit_bones[name].align_roll(roll)
    bpy.ops.object.mode_set(mode='OBJECT')
    for bone in arm.data.bones:
        bone.use_deform = True      # also root and the sockets: the export keeps deform bones only
    missing = [g.name for g in obj.vertex_groups if g.name not in arm.data.bones]
    if missing:
        raise RuntimeError("weights for unknown bones: %s" % missing)
    obj.parent = arm
    obj.modifiers.new("Armature", 'ARMATURE').object = arm
    return arm


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


# --- Texture: a plain mannequin with a readable front ---------------------------------------

def build_material(obj):
    """Light body (the game tints it), dark hands and feet, a dark band where the eyes are."""
    material = hb.new_material("M_" + NAME)
    nodes, links = material.node_tree.nodes, material.node_tree.links
    coords = nodes.new('ShaderNodeTexCoord')
    separate = nodes.new('ShaderNodeSeparateXYZ')
    links.new(coords.outputs['Object'], separate.inputs['Vector'])

    def ramp(socket, low, high):
        node = nodes.new('ShaderNodeMapRange')
        node.inputs['From Min'].default_value = low
        node.inputs['From Max'].default_value = high
        links.new(socket, node.inputs['Value'])
        return node.outputs['Result']

    def combine(operation, a, b):
        node = nodes.new('ShaderNodeMath')
        node.operation = operation
        for index, value in enumerate((a, b)):
            if isinstance(value, float):
                node.inputs[index].default_value = value
            else:
                links.new(value, node.inputs[index])
        return node.outputs['Value']

    abs_x = combine('ABSOLUTE', separate.outputs['X'], 0.0)
    # Distance along the forearm: grows towards the fingers; the wrist is the border.
    along_arm = combine('ADD', combine('MULTIPLY', abs_x, FOREARM_DIR.x), combine('MULTIPLY', separate.outputs['Z'], FOREARM_DIR.z))
    wrist = WRIST.x * FOREARM_DIR.x + WRIST.z * FOREARM_DIR.z
    hands = combine('MULTIPLY', ramp(along_arm, wrist - 2.0, wrist), ramp(abs_x, 30.0, 34.0))
    feet = ramp(separate.outputs['Z'], 11.5, 9.5)
    eyes = combine('MULTIPLY', combine('MULTIPLY', ramp(separate.outputs['Z'], 167.0, 168.2), ramp(separate.outputs['Z'], 172.8, 171.6)),
                   ramp(separate.outputs['Y'], -5.0, -7.0))
    dark = combine('MAXIMUM', combine('MAXIMUM', feet, hands), eyes)

    noise = nodes.new('ShaderNodeTexNoise')
    noise.inputs['Scale'].default_value = 0.08
    noise.inputs['Detail'].default_value = 3.0
    links.new(coords.outputs['Object'], noise.inputs['Vector'])
    base = nodes.new('ShaderNodeMix')
    base.data_type = 'RGBA'
    base.inputs['A'].default_value = (0.72, 0.70, 0.67, 1.0)
    base.inputs['B'].default_value = (0.82, 0.80, 0.78, 1.0)
    links.new(noise.outputs['Fac'], base.inputs['Factor'])
    final = nodes.new('ShaderNodeMix')
    final.data_type = 'RGBA'
    final.inputs['B'].default_value = (0.10, 0.10, 0.11, 1.0)
    links.new(dark, final.inputs['Factor'])
    links.new(base.outputs['Result'], final.inputs['A'])

    obj.data.materials.append(material)
    path = os.path.join(hb.ART_DIR, NAME, "T_%s_color.png" % NAME)
    hb.bake_color(obj, material, final.outputs['Result'], "T_%s_color" % NAME, 1024, path)
    print("build_humanoid: texture " + path)


# --- Poses ----------------------------------------------------------------------------------
# A pose is {bone: [(axis, degrees), ...]}: turns around the world axes, on top of what the
# parent bone does, in the order given.
#   "x": forward (+) / back (-): a hanging limb swings forward, the trunk and head bow forward;
#   "y": a hanging limb goes to the character's right (+) / left (-), so +y lifts the right
#        arm sideways and lowers the left one;
#   "z": turn to the character's left (+) / right (-).
AXES = {"x": Vector((1, 0, 0)), "y": Vector((0, 1, 0)), "z": Vector((0, 0, 1))}
UPRIGHT = {"pelvis", "spine_01", "spine_02", "spine_03", "neck_01", "head"}


def apply_pose(arm, pose, pelvis_offset=(0.0, 0.0, 0.0), ground=False):
    """Sets every pose bone. ground=True lowers or lifts the pelvis until a foot touches the floor."""
    for pbone in arm.pose.bones:                # creation order: parents before children
        pbone.rotation_mode = 'QUATERNION'
        rest = pbone.bone.matrix_local
        inherited = pbone.parent.matrix @ pbone.parent.bone.matrix_local.inverted() @ rest if pbone.parent else rest.copy()
        location = inherited.to_translation()
        rotation = inherited.to_3x3()
        for axis, degrees in pose.get(pbone.name, ()):
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
    for suffix, sign in (("_l", 1.0), ("_r", -1.0)):
        for bone, point in SOLE_POINTS:
            pbone = arm.pose.bones[bone + suffix]
            heights.append((pbone.matrix @ pbone.bone.matrix_local.inverted() @ side_vector(point, sign)).z)
    return min(heights)


def over(base, **bones):
    """base with the given bones replaced."""
    result = dict(base)
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
GUARD = {
    "pelvis": [("z", -18)], "spine_01": [("x", 6)], "spine_02": [("z", -6)], "neck_01": [("z", 12)], "head": [("z", 12)],
    "thigh_l": [("x", 26)], "calf_l": [("x", -32)], "foot_l": [("x", 6)],
    "thigh_r": [("x", -14)], "calf_r": [("x", -30)], "foot_r": [("x", 30)],
    "upperarm_r": [("y", -30), ("x", 8)], "lowerarm_r": [("x", 80)], "hand_r": [("x", -30)],
    "upperarm_l": [("y", 32), ("x", 30)], "lowerarm_l": [("x", 60), ("z", -35)],
}
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
    falling = {"pelvis": [("x", -38)], "spine_01": [("x", -8)], "head": [("x", 14)],
               "thigh_l": [("x", 40)], "calf_l": [("x", -60)], "thigh_r": [("x", 16)], "calf_r": [("x", -36)],
               "upperarm_l": [("y", -30), ("x", 30)], "upperarm_r": [("y", 30), ("x", 30)], "lowerarm_l": [("x", 30)], "lowerarm_r": [("x", 30)]}
    lying = {"pelvis": [("x", -88)], "head": [("x", 6)],
             "thigh_l": [("x", 14), ("z", 8)], "calf_l": [("x", -22)], "thigh_r": [("z", -8)], "calf_r": [("x", -4)],
             "upperarm_l": [("y", -25), ("x", -6)], "upperarm_r": [("y", 35), ("x", -6)], "lowerarm_l": [("y", -30)], "lowerarm_r": [("y", 20)]}
    key_pose(arm, 0, GUARD)
    key_pose(arm, 7, falling, (0.0, 14.0, -34.0), ground=False)
    key_pose(arm, 13, lying, (0.0, 30.0, -86.0), ground=False)
    key_pose(arm, 16, lying, (0.0, 30.0, -87.0), ground=False)

    arm.animation_data.action = None
    apply_pose(arm, {})


# --- Report and previews --------------------------------------------------------------------

def report(obj, arm):
    mesh = obj.data
    quads = sum(1 for p in mesh.polygons if len(p.vertices) == 4)
    triangles = sum(1 for p in mesh.polygons if len(p.vertices) == 3)
    bm = bmesh.new()
    bm.from_mesh(mesh)
    open_edges = sum(1 for e in bm.edges if not e.is_manifold)
    poles = sum(1 for v in bm.verts if len(v.link_edges) > 5)
    bm.free()
    places = {(round(v.co.x, 2), round(v.co.y, 2), round(v.co.z, 2)) for v in mesh.vertices}
    lopsided = sum(1 for x, y, z in places if (-x if x else 0.0, y, z) not in places)
    influences = max(len(v.groups) for v in mesh.vertices)
    size = obj.dimensions
    print("build_humanoid: size %.0f x %.0f x %.0f cm, %d vertices, %d faces (%d quads, %d triangles, %d n-gons), "
          "%d open edges, %d vertices with more than 5 edges, %d vertices without a mirror partner, at most %d bones per vertex"
          % (size.x, size.y, size.z, len(mesh.vertices), len(mesh.polygons), quads, triangles, len(mesh.polygons) - quads - triangles,
             open_edges, poles, lopsided, influences))
    print("build_humanoid: %d bones (%s), shape keys: %s, actions: %s"
          % (len(arm.data.bones), ", ".join(b.name for b in arm.data.bones), ", ".join(k.name for k in mesh.shape_keys.key_blocks[1:]),
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


def preview(obj, arm, props):
    keys = obj.data.shape_keys.key_blocks

    def set_preset(preset):
        for key in keys[1:]:
            key.value = 1.0 if key.name == preset else 0.0

    def show_props(visible):
        for prop in props:
            prop.hide_render = not visible

    turnaround = (("front", 0.0), ("front_left", -35.0), ("side", -90.0), ("back", 180.0))
    two = (("front_left", -35.0), ("side", -90.0))
    arm.animation_data.action = None

    # The three presets in the rest pose, without the props.
    show_props(False)
    apply_pose(arm, {})
    for preset in ("neutral",) + PRESET_KEYS:
        set_preset(preset)
        hb.render_preview("%s_rest_%s" % (NAME, preset), (0, 0, 92), 330, size=(600, 800), views=turnaround)

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

    # Key poses of the placeholder animations, with sword and shield.
    show_props(True)
    shots = (("Idle", 0), ("Move", 0), ("Move", 4), ("Windup", 8), ("Strike", 4), ("Charge", 0), ("HeavyStrike", 4), ("Recovery", 5),
             ("Block", 4), ("Dodge", 5), ("Jump", 0), ("Death", 7), ("Death", 16))
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
    obj = build_mesh()
    arm = build_rig(obj)
    build_material(obj)
    build_shape_keys(obj)
    props = build_props(arm)
    build_animations(arm)
    report(obj, arm)

    hb.save_blend(os.path.join(NAME, NAME + ".blend"))
    hb.export_model(NAME, arm, [obj])
    hb.export_actions(NAME, arm)
    if "nopreview" not in hb.script_args():
        preview(obj, arm, props)
    print("build_humanoid: done")


main()
