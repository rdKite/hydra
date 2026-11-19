"""Builds the humanoid base model: one skeleton for all humanoids, a stylised human body with
a simple outfit, the three body presets as shape keys, hand sockets, placeholder animations,
FBX export.

  blender -b --python Art/scripts/build_humanoid.py            (add "-- nopreview" to skip the renders)

What it is (roadmap M5, first item; decisions of 2026-11-19: one shared humanoid base model
for all races, gender-fluid sliders). Detail level: a stylised game character (simplified
muscle masses, a modelled face, five fingers), not a final character design.

How the body is made
- Cage: quads written out ring by ring – trunk, neck and head as one stack of half rings
  (24 points round), 10-sided arms out of a hole in the side of the chest, a flat palm with
  four fingers and a thumb (4-sided), 16-sided legs out of the groin, 8-sided feet. Mirrored,
  then two Catmull-Clark levels.
- Sculpt: muscle masses, face (eye sockets, nose, lips, chin, cheeks, ears) and the outfit
  (hair volume, shirt, belt, boots) are pushed along the vertex normals by smooth ellipsoid
  bumps and region masks. Eyeballs are separate quad spheres in the same mesh.
- Weights are set per cage ring (each ring knows its bones) and smoothed by the
  subdivision; at most 4 bones per vertex.
- Body presets: the cage measures and the sculpt gains differ; "male" and "female" are shape
  keys (morph targets), the neutral preset is the base shape. Joints never move, so one
  skeleton and one set of animations fits all presets.
- Texture: the object position of every texel is baked once (float image), the colours are
  computed from it in numpy (skin, hair, brows, eyes, lips, shirt, trousers, belt, boots).
  The alpha channel is the tint mask: 1 where the side colour may tint (the shirt).

Skeleton: bone names of the Unreal mannequin (root, pelvis, spine_01..03, neck_01, head,
clavicle / upperarm / lowerarm / hand, thumb / index / middle / ring / pinky _01.._03,
thigh / calf / foot / ball; _l and _r), A-pose. Bone axes are Blender's (Y along the bone).
Socket bones: weapon_r (child of hand_r, in the fist, +Y along the blade) and shield_l (child
of lowerarm_l, +Y out of the shield's face).

Writes Art/humanoid/humanoid.blend (with a stand-in longsword and round shield on the
sockets, for animating; they are not exported), T_humanoid_color.png,
Art/export/humanoid/*.fbx and preview renders Saved/ArtPreview/humanoid_*.png.
"""
import math
import os
import sys

import bmesh
import bpy
import numpy as np
import mathutils.kdtree
from mathutils import Matrix, Vector

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import hydra_blender as hb  # noqa: E402

NAME = "humanoid"
SUBDIVISION = 2
TEXTURE_SIZE = 2048
PRESET_KEYS = ("male", "female")     # the neutral preset is the base shape


def ramp(x, a, b):
    """0 at a, 1 at b, linear in between (a may be larger than b)."""
    return max(0.0, min(1.0, (x - a) / (b - a)))


def np_smooth(x, a, b):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def side_vector(v, sign):
    return Vector((v.x * sign, v.y, v.z))


def arm_frame(direction):
    """Axes of a limb ring: along the limb, across it in the arm plane (up and out), to the back."""
    d = direction.normalized()
    u = Vector((-d.z, 0.0, d.x)).normalized()
    return d, u, u.cross(d)


# --- Joints (centimetres, facing -Y, the character's left is +X; only the left side) --------
ARM_DIR = Vector((math.sin(math.radians(40.0)), 0.0, -math.cos(math.radians(40.0))))   # A-pose
FOREARM_DIR = Vector((0.62, -0.10, -0.78)).normalized()
SHOULDER = Vector((17.2, 0.4, 141.5))
ELBOW = SHOULDER + ARM_DIR * 28.5
WRIST = ELBOW + FOREARM_DIR * 25.5
HIP = Vector((10.2, 0.6, 92.0))
KNEE = Vector((11.0, -0.8, 50.5))
ANKLE = Vector((11.1, 1.6, 8.5))
BALL = Vector((11.5, -14.5, 2.6))
TOE = Vector((11.8, -21.5, 2.6))
EYE = Vector((3.25, -7.35, 168.6))
EYE_RADIUS = 1.25
# Points under the foot that may touch the floor: (bone, rest position).
SOLE_POINTS = (("foot", Vector((11.1, 6.0, 0.0))), ("foot", Vector((11.5, -14.5, 0.0))), ("ball", Vector((11.8, -20.0, 0.0))))

# Hand axes: along the hand, out of the back of the hand, towards the pinky side (back).
HD, HU, HV = arm_frame(FOREARM_DIR)


def hand_point(t, across, out):
    return WRIST + HD * t + HV * across + HU * out


def palm_rows(t, half_width, back, palm, arc=0.0, drop=0.0):
    """Two rows of 5 points across the hand (thumb side first): back of the hand, palm."""
    back_row, palm_row = [], []
    for i in range(5):
        s = -1.0 + 0.5 * i
        along = t - arc * s * s - drop * max(0.0, s)
        f = 1.0 - 0.3 * s * s
        back_row.append(hand_point(along, half_width * s, back * f))
        palm_row.append(hand_point(along, half_width * s, -palm * f))
    return back_row, palm_row


# Palm rings: near the wrist, middle of the palm, knuckles.
PALM = [palm_rows(1.5, 3.7, 2.0, 2.1), palm_rows(5.3, 4.4, 2.0, 2.4), palm_rows(9.5, 4.5, 1.7, 1.8, arc=0.8, drop=0.4)]
FINGERS = (("index", 7.6, -4.0), ("middle", 8.4, 0.0), ("ring", 7.9, 3.5), ("pinky", 6.4, 8.0))   # name, length, spread (deg)
THUMB_DIR = (-HV * 0.78 + HD * 0.5 - HU * 0.38).normalized()
THUMB_LENGTH = 8.4
GRIP = hand_point(9.0, 0.0, -2.6)      # inside the fist: where a sword's grip sits


def palm_ring(index):
    back, palm = PALM[index]
    return back + palm[::-1]          # 0..4 back of the hand, 5..9 palm; 9 -> 0 is the thumb side


def finger_frame(i):
    name, length, spread = FINGERS[i]
    back, palm = PALM[2]
    base = (back[i] + back[i + 1] + palm[i] + palm[i + 1]) / 4.0
    a = math.radians(spread)
    along = (HD * math.cos(a) + HV * math.sin(a)).normalized()
    side = (HV * math.cos(a) - HD * math.sin(a)).normalized()
    up = (HU - along * HU.dot(along)).normalized()
    return name, length, base, along, side, up


def finger_joints(i):
    name, length, base, along, _, _ = finger_frame(i)
    l1, l2 = 0.45 * length, 0.30 * length
    return name, [base, base + along * l1, base + along * (l1 + l2), base + along * (length - 0.2)]


def thumb_frame():
    r1, r2 = palm_ring(0), palm_ring(1)
    loop = [r1[9], r1[0], r2[0], r2[9]]
    base = sum(loop, Vector()) / 4.0
    a1 = (HU - THUMB_DIR * HU.dot(THUMB_DIR)).normalized()
    a2 = THUMB_DIR.cross(a1)
    if a2.dot(HD) < 0.0:
        a2 = -a2
    return base, a1, a2


def thumb_joints():
    base, _, _ = thumb_frame()
    return [base - THUMB_DIR * 1.6, base + THUMB_DIR * 3.0, base + THUMB_DIR * 5.9, base + THUMB_DIR * (THUMB_LENGTH - 0.2)]


# --- Measures of the body -------------------------------------------------------------------
# Trunk, neck and head: half rings of 13 points from the front centre (j=0) round the left
# side to the back centre (j=12). z, half width, depth to the front, depth to the back,
# centre y, squareness (2 = ellipse, more = boxier).
TRUNK = [
    (86.0, 17.6, 8.8, 10.6, 0.6, 2.6),      # 0 groin: the legs start here
    (93.0, 18.2, 9.2, 11.8, 0.8, 2.6),      # 1 hips, seat
    (100.0, 17.0, 9.4, 10.8, 0.6, 2.6),     # 2 hip bones
    (106.0, 15.2, 9.4, 9.6, 0.3, 2.6),
    (112.0, 14.6, 9.6, 9.0, 0.1, 2.6),      # 4 waist
    (119.0, 15.4, 10.2, 9.4, 0.0, 2.6),
    (128.0, 16.9, 11.4, 10.3, 0.0, 2.6),    # 6 armpit: lower edge of the arm hole
    (134.0, 17.4, 11.6, 10.5, 0.0, 2.6),    # 7 chest
    (140.0, 17.6, 10.6, 10.2, 0.0, 2.6),
    (145.5, 16.6, 8.6, 8.8, 0.2, 2.6),      # 9 top of the shoulder: upper edge of the arm hole
    (149.5, 12.0, 6.8, 7.6, 0.6, 2.4),      # 10 trapezius
    (152.5, 6.6, 5.6, 6.0, 0.6, 2.2),       # 11 base of the neck
    (155.5, 5.6, 5.2, 5.6, 0.8, 2.1),       # 12 neck
    (157.8, 6.6, 8.2, 5.8, 0.4, 2.2),       # 13 under the jaw (chin in front)
    (160.5, 7.4, 9.4, 7.6, 0.2, 2.2),       # 14 mouth, jaw
    (164.5, 8.2, 9.8, 9.4, 0.0, 2.2),       # 15 nose base, cheeks
    (168.5, 8.6, 9.5, 10.2, 0.0, 2.2),      # 16 eyes
    (172.5, 8.5, 9.3, 10.4, 0.2, 2.2),      # 17 brows
    (176.0, 7.9, 8.4, 9.8, 0.4, 2.2),
    (179.0, 6.5, 6.6, 8.0, 0.6, 2.2),
]
CROWN = Vector((0.0, 1.0, 181.3))
TRUNK_ANGLES = (0, 15, 30, 44, 56, 66, 90, 114, 124, 136, 150, 165, 180)   # wider at the side, for the arm hole
HEAD_ANGLES = tuple(15 * j for j in range(13))
NECK_LAYERS = (11, 12)
# The arm hole: faces of rows 6..8 between the columns 5..7 are left out.
HOLE_ROWS, HOLE_COLUMNS = (6, 9), (5, 7)
CROTCH = (Vector((0.0, 7.0, 82.5)), Vector((0.0, 0.8, 80.5)), Vector((0.0, -5.5, 82.0)))   # back, middle, front

# Upper arm rings: distance from the shoulder joint, radius across (u), radius front-back
# (v), how far the ring still faces the side (1 = the plane of the arm hole), shift outwards.
UPPER_ARM = [(7.0, 6.3, 6.2, 0.55, 1.2), (12.5, 5.7, 5.7, 0.0, 0.0), (18.5, 5.3, 5.4, 0.0, 0.0), (24.0, 4.9, 4.9, 0.0, 0.0), (28.5, 4.5, 4.6, 0.0, 0.0)]
DELTOID_RING = (Vector((19.9, 0.4, 137.2)), 8.0, 6.4)       # first ring, in the plane of the arm hole
FOREARM = [(3.5, 4.8, 4.6), (9.0, 4.6, 4.3), (15.5, 3.9, 3.6), (21.0, 3.0, 3.1), (25.5, 2.4, 3.2)]
# Leg rings: z, centre x, centre y, radius out/in, radius to the front, radius to the back.
LEG = [
    (76.0, 10.3, 0.6, 9.2, 8.4, 9.4),
    (68.0, 10.8, 0.1, 8.6, 8.2, 8.4),
    (60.5, 11.0, -0.4, 7.4, 7.4, 7.0),
    (55.0, 11.0, -0.7, 6.5, 6.8, 6.1),
    (50.5, 11.0, -0.8, 6.0, 6.4, 5.8),      # knee
    (46.5, 11.0, -0.4, 5.9, 5.8, 6.0),
    (40.5, 11.1, 0.4, 5.9, 5.0, 7.0),       # calf
    (33.5, 11.1, 0.8, 5.5, 4.6, 6.5),
    (27.0, 11.1, 1.0, 5.1, 4.5, 5.6),       # top of the boot
    (19.0, 11.1, 1.2, 4.7, 4.5, 4.9),
    (13.0, 11.1, 1.4, 4.5, 4.5, 4.8),
    (8.5, 11.1, 1.6, 4.7, 4.8, 5.2),        # ankle
    (0.0, 11.1, 1.8, 4.9, 5.4, 5.0),        # sole under the heel
]
# Foot rings in front of the ankle: y, height, half width, centre x.
FOOT = [(-6.5, 7.2, 4.9, 11.2), (-11.0, 5.6, 5.3, 11.4), (-15.0, 4.5, 5.4, 11.6), (-18.8, 3.7, 5.0, 11.8), (-21.6, 2.9, 4.0, 11.9)]

# How much a ring with n points shrinks under subdivision (the limit surface lies inside).
def k_ring(n):
    return 3.0 / (2.0 + math.cos(2.0 * math.pi / n))


# Body presets: what differs from the neutral body. trunk: per layer (half width, depth front,
# depth back) in centimetres; arm / thigh / calf / neck: factors on the ring radii; gain:
# factors on the sculpt groups (see sculpt_features).
PRESETS = {
    "neutral": {},
    "male": {
        "trunk": {1: (-0.6, 0.0, -0.6), 2: (-0.4, 0.0, 0.0), 3: (0.8, 0.4, 0.4), 4: (1.4, 0.6, 0.5), 5: (1.8, 0.8, 0.7),
                  6: (1.6, 0.9, 0.9), 7: (1.6, 1.0, 0.9), 8: (1.0, 0.8, 0.8), 9: (0.8, 0.5, 0.6), 10: (1.2, 0.3, 0.4),
                  13: (0.4, 0.2, 0.2), 14: (0.5, 0.2, 0.2)},
        "arm": 1.08, "thigh": 1.04, "calf": 1.07, "neck": 1.12,
        "gain": {"muscle": 1.4, "pec": 1.7, "abs": 1.4, "trap": 1.6, "lat": 1.5, "glute": 0.85, "male": 1.0,
                 "brow": 1.4, "jaw": 1.6, "cheek": 0.9, "lips": 0.85},
    },
    "female": {
        "trunk": {0: (1.0, 0.0, 0.8), 1: (2.0, 0.2, 1.6), 2: (1.2, 0.1, 0.6), 3: (-1.0, -0.6, -0.3), 4: (-2.0, -0.8, -0.5),
                  5: (-1.6, -0.4, -0.4), 6: (-1.4, 0.0, -0.4), 7: (-1.2, 0.0, -0.3), 8: (-1.1, -0.2, -0.3), 9: (-1.0, -0.1, 0.0),
                  10: (-0.6, 0.0, 0.0), 13: (-0.4, -0.2, 0.0), 14: (-0.4, -0.1, 0.0)},
        "arm": 0.92, "thigh": 1.06, "calf": 0.95, "neck": 0.9,
        "gain": {"muscle": 0.55, "pec": 0.3, "abs": 0.4, "trap": 0.6, "lat": 0.6, "glute": 1.4, "bust": 1.0,
                 "brow": 0.7, "jaw": 0.6, "cheek": 1.15, "lips": 1.25},
    },
}

# --- Skin weights ---------------------------------------------------------------------------
SPINE = [(103.5, "pelvis", "spine_01", 4.0), (117.5, "spine_01", "spine_02", 4.0), (131.0, "spine_02", "spine_03", 4.0),
         (151.5, "spine_03", "neck_01", 2.0), (157.0, "neck_01", "head", 2.0)]


def spine_weights(z):
    for boundary, lower, upper, half in SPINE:
        if z < boundary + half:
            if z <= boundary - half:
                return {lower: 1.0}
            t = (z - (boundary - half)) / (2.0 * half)
            return {lower: 1.0 - t, upper: t}
    return {"head": 1.0}


def trunk_weights(k, j, co, angles):
    s = math.sin(math.radians(angles[j]))     # 0 at the centre line, 1 at the side
    c = math.cos(math.radians(angles[j]))     # 1 in front, -1 at the back
    if k == 13 and c > 0.0:
        return {"head": 1.0}                  # chin
    weights = {}
    if k == 0:
        weights["thigh_l"] = (0.42 if c > -0.3 else 0.16) * (0.35 + 0.65 * s)   # less on the seat: no ridge in a squat
    elif k == 1:
        weights["thigh_l"] = 0.14 * s * (1.0 if c > -0.3 else 0.5)
    if 6 <= k <= 9:
        weights["upperarm_l"] = 0.32 * hb.smoothstep(0.6, 1.0, s) * ramp(co.z, 126.0, 140.0)
    if 9 <= k <= 11:
        weights["clavicle_l"] = 0.55 * ramp(co.x, 3.5, 12.0) * (1.0 if k < 11 else 0.5)
    rest = 1.0 - sum(weights.values())
    for bone, share in spine_weights(co.z).items():
        weights[bone] = weights.get(bone, 0.0) + share * rest
    return weights


def leg_weights(z):
    if z >= 72.0:
        return {"thigh_l": 0.85, "pelvis": 0.15} if z > 74.0 else {"thigh_l": 1.0}
    knee = ramp(z, 56.0, 45.0)
    if knee < 1.0:
        return {"thigh_l": 1.0 - knee, "calf_l": knee}
    foot = ramp(z, 13.5, 5.0)
    return {"calf_l": 1.0 - foot, "foot_l": foot}


# --- Cage -----------------------------------------------------------------------------------

class Cage:
    """Left half of the body as quads with weights per vertex; mirror() adds the right half."""

    def __init__(self):
        self.verts, self.weights, self.faces = [], [], []

    def vert(self, co, weights):
        self.verts.append(Vector(co))
        self.weights.append({b: w for b, w in weights.items() if w > 1e-4})
        return len(self.verts) - 1

    def quad(self, a, b, c, d):
        self.faces.append((a, b, c, d))

    def aligned(self, lower, upper):
        """upper re-ordered so that it runs like lower (best offset and direction)."""
        n = len(lower)
        a = [self.verts[i] for i in lower]
        b = [self.verts[i] for i in upper]
        ca, cb = sum(a, Vector()) / n, sum(b, Vector()) / n
        sa = max((p - ca).length for p in a) or 1.0
        sb = max((p - cb).length for p in b) or 1.0
        best = None
        for direction in (1, -1):
            for offset in range(n):
                cost = sum(((a[m] - ca) / sa - (b[(offset + direction * m) % n] - cb) / sb).length_squared for m in range(n))
                if best is None or cost < best[0]:
                    best = (cost, offset, direction)
        _, offset, direction = best
        return [upper[(offset + direction * m) % n] for m in range(n)]

    def bridge(self, lower, upper, align=True, skip=()):
        if align:
            upper = self.aligned(lower, upper)
        n = len(lower)
        for m in range(n):
            if m not in skip:
                self.quad(lower[m], lower[(m + 1) % n], upper[(m + 1) % n], upper[m])
        return upper

    def ring(self, points, weights):
        return [self.vert(p, weights) for p in points]

    def cap(self, ring, centre, weights):
        middle = self.vert(centre, weights)
        for m in range(0, len(ring), 2):
            self.quad(ring[m - 1], ring[m], ring[(m + 1) % len(ring)], middle)

    def mirror(self):
        mapping = {}
        for index in range(len(self.verts)):
            co = self.verts[index]
            if abs(co.x) < 1e-4:
                co.x = 0.0
                mapping[index] = index
                shared = {}                   # on the centre line: a left weight is split between both sides
                for bone, weight in self.weights[index].items():
                    if bone.endswith("_l"):
                        for name in (bone, bone[:-2] + "_r"):
                            shared[name] = shared.get(name, 0.0) + weight * 0.5
                    else:
                        shared[bone] = shared.get(bone, 0.0) + weight
                self.weights[index] = shared
            else:
                swapped = {(b[:-2] + "_r" if b.endswith("_l") else b): w for b, w in self.weights[index].items()}
                mapping[index] = self.vert((-co.x, co.y, co.z), swapped)
        self.faces += [tuple(mapping[i] for i in reversed(face)) for face in self.faces]


def ellipse(centre, a, b, ra, rb, count, start_deg=0.0):
    k = k_ring(count)
    return [centre + (a * (math.cos(t) * ra) + b * (math.sin(t) * rb)) * k
            for t in (math.radians(start_deg + m * 360.0 / count) for m in range(count))]


def square(centre, a, b, ha, hb_):
    """4 corners: (-a,-b), (+a,-b), (+a,+b), (-a,+b), sized so that the subdivided tube has about these half sizes."""
    k = 1.08
    return [centre + a * (sa * ha * k) + b * (sb * hb_ * k) for sa, sb in ((-1, -1), (1, -1), (1, 1), (-1, 1))]


def trunk_point(k, j, spec, angles):
    z, w, df, db, cy, p = TRUNK[k]
    dw, ddf, ddb = spec.get("trunk", {}).get(k, (0.0, 0.0, 0.0))
    w, df, db = w + dw, df + ddf, db + ddb
    if k in NECK_LAYERS:
        f = spec.get("neck", 1.0)
        w, df, db = w * f, df * f, db * f
    phi = math.radians(angles[j])
    s, c = math.sin(phi), math.cos(phi)
    e = 2.0 / p
    x = w * abs(s) ** e
    y = cy - (df if c >= 0.0 else db) * math.copysign(abs(c) ** e, c)
    return Vector((x, y, z))


def build_cage(preset):
    spec = PRESETS[preset]
    cage = Cage()

    # Trunk, neck and head.
    layers = []
    for k in range(len(TRUNK)):
        angles = TRUNK_ANGLES if k <= 10 else HEAD_ANGLES
        layer = {}
        for j in range(13):
            if HOLE_ROWS[0] < k < HOLE_ROWS[1] and j == 6:
                continue                                         # inside the arm hole
            co = trunk_point(k, j, spec, angles)
            layer[j] = cage.vert(co, trunk_weights(k, j, co, angles))
        layers.append(layer)
    for k in range(len(TRUNK) - 1):
        for j in range(12):
            if HOLE_ROWS[0] <= k < HOLE_ROWS[1] and HOLE_COLUMNS[0] <= j < HOLE_COLUMNS[1]:
                continue
            cage.quad(layers[k][j], layers[k][j + 1], layers[k + 1][j + 1], layers[k + 1][j])
    crown = cage.vert(CROWN, {"head": 1.0})
    top = layers[-1]
    for j in range(1, 12, 2):
        cage.quad(top[j - 1], top[j], top[j + 1], crown)

    # Arm, out of the hole in the side of the chest.
    r0, r1 = HOLE_ROWS
    c0, c1 = HOLE_COLUMNS
    hole = [layers[r0][c0], layers[r0][6], layers[r0][c1]] + [layers[k][c1] for k in range(r0 + 1, r1 + 1)] \
        + [layers[r1][6], layers[r1][c0]] + [layers[k][c0] for k in range(r1 - 1, r0, -1)]
    arm_scale = spec.get("arm", 1.0)
    centre, ru, rv = DELTOID_RING
    below = cage.ring(ellipse(centre, Vector((0, 0, 1)), Vector((0, 1, 0)), ru * arm_scale, rv * arm_scale, 10),
                      {"upperarm_l": 0.55, "clavicle_l": 0.3, "spine_03": 0.15})
    below = cage.bridge(hole, below)
    upper_weights = [{"upperarm_l": 0.85, "clavicle_l": 0.15}, {"upperarm_l": 1.0}, {"upperarm_l": 1.0},
                     {"upperarm_l": 0.85, "lowerarm_l": 0.15}, {"upperarm_l": 0.5, "lowerarm_l": 0.5}]
    for (t, ru, rv, side, out), weights in zip(UPPER_ARM, upper_weights):
        d, u, v = arm_frame(ARM_DIR * (1.0 - side) + Vector((side, 0.0, 0.0)))
        centre = SHOULDER + ARM_DIR * t + Vector((out, 0.0, 0.0))
        ring = cage.ring(ellipse(centre, u, v, ru * arm_scale, rv * arm_scale, 10), weights)
        below = cage.bridge(below, ring)
    d, u, v = arm_frame(FOREARM_DIR)
    fore_weights = [{"upperarm_l": 0.2, "lowerarm_l": 0.8}, {"lowerarm_l": 1.0}, {"lowerarm_l": 1.0},
                    {"lowerarm_l": 0.9, "hand_l": 0.1}, {"lowerarm_l": 0.5, "hand_l": 0.5}]
    for (t, ru, rv), weights in zip(FOREARM, fore_weights):
        scale = arm_scale if t < 20.0 else 1.0
        ring = cage.ring(ellipse(ELBOW + d * t, u, v, ru * scale, rv * scale, 10), weights)
        below = cage.bridge(below, ring)

    # Hand: palm rings, thumb out of the side, fingers out of the knuckle ring.
    palm1 = cage.ring(palm_ring(0), {"hand_l": 0.85, "lowerarm_l": 0.15})
    cage.bridge(below, palm1)
    palm2 = cage.ring(palm_ring(1), {"hand_l": 1.0})
    cage.bridge(palm1, palm2, align=False, skip=(9,))
    knuckle_points = palm_ring(2)
    knuckles = []
    for index, point in enumerate(knuckle_points):
        column = index if index < 5 else 9 - index               # 0..4 from the thumb side
        touching = [f for f in (column - 1, column) if 0 <= f < 4]
        weights = {"hand_l": 0.5}
        for f in touching:
            weights[FINGERS[f][0] + "_01_l"] = 0.5 / len(touching)
        knuckles.append(cage.vert(point, weights))
    cage.bridge(palm2, knuckles, align=False)

    base, a1, a2 = thumb_frame()
    below = [palm1[9], palm1[0], palm2[0], palm2[9]]
    thumb = [(1.4, 1.5, 1.6, {"hand_l": 0.3, "thumb_01_l": 0.7}), (2.6, 1.35, 1.4, {"thumb_01_l": 0.7, "thumb_02_l": 0.3}),
             (3.5, 1.3, 1.35, {"thumb_01_l": 0.3, "thumb_02_l": 0.7}), (5.5, 1.15, 1.2, {"thumb_02_l": 0.7, "thumb_03_l": 0.3}),
             (6.4, 1.1, 1.15, {"thumb_02_l": 0.3, "thumb_03_l": 0.7}), (7.8, 0.95, 1.0, {"thumb_03_l": 1.0})]
    for t, h1, h2, weights in thumb:
        ring = cage.ring(square(base + THUMB_DIR * t, a1, a2, h1, h2), weights)
        below = cage.bridge(below, ring)
    cage.quad(*below)

    for i in range(4):
        name, length, fbase, along, side, up = finger_frame(i)
        l1, l2 = 0.45 * length, 0.30 * length
        bones = [name + "_0%d_l" % n for n in (1, 2, 3)]
        stations = [(l1 * 0.5, 0.95, 1.0, {bones[0]: 1.0}),
                    (l1 - 0.55, 0.88, 0.95, {bones[0]: 0.7, bones[1]: 0.3}), (l1 + 0.55, 0.85, 0.92, {bones[0]: 0.3, bones[1]: 0.7}),
                    (l1 + l2 - 0.45, 0.8, 0.86, {bones[1]: 0.7, bones[2]: 0.3}), (l1 + l2 + 0.45, 0.78, 0.84, {bones[1]: 0.3, bones[2]: 0.7}),
                    (length - 0.5, 0.7, 0.76, {bones[2]: 1.0})]
        below = [knuckles[i], knuckles[i + 1], knuckles[8 - i], knuckles[9 - i]]
        for t, hu, hv, weights in stations:
            ring = cage.ring(square(fbase + along * t, up, side, hu, hv), weights)
            below = cage.bridge(below, ring)
        cage.quad(*below)

    # Leg, out of the groin layer and the crotch.
    crotch = [cage.vert(co, {"pelvis": 0.5, "thigh_l": 0.5}) for co in CROTCH]
    below = [layers[0][j] for j in range(13)] + crotch
    rings = []
    for z, cx, cy, rs, rf, rb in LEG:
        scale = spec.get("thigh", 1.0) if z >= 57.0 else spec.get("calf", 1.0) if 30.0 <= z < 47.0 else 1.0
        points = []
        for m in range(16):
            a = math.radians(-33.75 + 22.5 * m)              # 0 = to the front, then to the outside
            radius = rf if math.cos(a) >= 0.0 else rb
            points.append(Vector((cx + math.sin(a) * rs * scale * k_ring(16), cy - math.cos(a) * radius * scale * k_ring(16), z)))
        rings.append(cage.ring(points, leg_weights(z) if z > 0.0 else {"foot_l": 1.0}))
    below = cage.bridge(below, rings[0])
    for previous, ring in zip(rings[:-2], rings[1:-1]):
        cage.bridge(previous, ring, align=False)
    ankle, sole = rings[-2], rings[-1]
    cage.bridge(ankle, sole, align=False, skip=(0, 1, 2))     # the front stays open for the foot
    cage.cap(sole, Vector((LEG[-1][1], LEG[-1][2], 0.0)), {"foot_l": 1.0})

    # Foot: 8-sided rings to the toe.
    below = ankle[0:4] + sole[3::-1]
    for y, h, w, cx in FOOT:
        ball = ramp(-y, 12.0, 17.0)
        weights = {"foot_l": 1.0 - ball, "ball_l": ball}
        w *= 1.06
        points = [(cx - 0.72 * w, y, 0.9 * h), (cx, y, h), (cx + 0.72 * w, y, 0.9 * h), (cx + w, y, 0.45 * h),
                  (cx + 0.8 * w, y, 0.0), (cx, y, 0.0), (cx - 0.8 * w, y, 0.0), (cx - w, y, 0.45 * h)]
        ring = cage.ring([Vector(p) for p in points], weights)
        below = cage.bridge(below, ring)
    y, h, _, cx = FOOT[-1]
    cage.cap(below, Vector((cx, y - 1.2, 0.42 * h)), {"ball_l": 1.0})

    cage.mirror()
    return cage


# --- Sculpt: muscle masses, face, outfit ----------------------------------------------------

def bump(center, radii, amp, group="body", axes=None):
    return (Vector(center), Vector(radii), amp, group, axes)


def sculpt_features():
    """Smooth ellipsoid bumps (left side; mirrored). amp in cm along the normal, - = inwards."""
    ud, uu, uv = arm_frame(ARM_DIR)
    fd, fu, fv = arm_frame(FOREARM_DIR)
    upper = (uu, uv, ud)        # radii: across, front-back, along the arm
    fore = (fu, fv, fd)
    crease = (Vector((0.67, 0.0, 0.74)), Vector((0, 1, 0)), Vector((-0.74, 0.0, 0.67)))    # groin crease, diagonal
    neck = (Vector((1, 0, 0)), Vector((0, 0.86, -0.51)).normalized(), Vector((-0.27, -0.42, -0.87)).normalized())
    features = [
        # chest and belly
        bump((7.2, -11.2, 133.5), (6.4, 3.0, 4.6), 1.0, "pec"),
        bump((7.6, -11.4, 128.8), (6.0, 3.0, 1.3), -0.35, "pec"),
        bump((0.0, -11.8, 133.0), (1.3, 3.0, 6.5), -0.35, "muscle"),         # breastbone
        bump((7.4, -12.0, 129.5), (5.0, 4.0, 4.6), 2.6, "bust"),
        bump((2.7, -10.2, 122.0), (2.3, 3.0, 2.4), 0.35, "abs"),
        bump((2.7, -10.0, 116.6), (2.3, 3.0, 2.4), 0.35, "abs"),
        bump((2.7, -9.8, 111.2), (2.3, 3.0, 2.4), 0.3, "abs"),
        bump((0.0, -10.2, 116.0), (0.8, 3.0, 9.0), -0.3, "abs"),
        bump((0.0, -9.9, 108.6), (0.6, 1.6, 0.7), -0.6),                      # navel
        bump((12.4, -5.5, 106.0), (4.0, 5.0, 5.0), 0.35, "muscle"),           # obliques
        bump((7.5, -8.0, 89.0), (6.0, 3.0, 1.0), -0.4, "body", crease),       # groin crease
        bump((0.0, -9.4, 104.5), (7.0, 3.0, 5.5), 0.25),                      # lower belly
        # back
        bump((13.4, 4.5, 127.0), (4.0, 5.0, 8.0), 0.8, "lat"),
        bump((8.0, 10.6, 137.0), (4.6, 3.0, 5.0), 0.55, "muscle"),            # shoulder blades
        bump((0.0, 10.8, 124.0), (1.4, 3.0, 16.0), -0.6),                     # spine furrow
        bump((7.0, 3.5, 149.5), (5.0, 5.0, 3.0), 0.7, "trap"),
        bump((9.0, -6.6, 148.0), (6.5, 1.6, 1.0), 0.35),                      # collarbone
        bump((8.0, -4.0, 150.5), (2.8, 3.0, 1.6), -0.4),                      # hollow above it
        bump((8.2, 11.6, 90.0), (7.5, 5.0, 7.0), 1.5, "glute"),
        bump((8.4, 9.6, 80.5), (6.0, 4.0, 1.3), -0.6, "glute"),               # fold under the seat
        # arms
        bump(SHOULDER + ud * 3.5 + uu * 2.5, (5.5, 6.0, 6.5), 0.8, "muscle", upper),     # deltoid
        bump(SHOULDER + ud * 17.0 - uv * 3.0, (3.2, 2.6, 6.0), 0.9, "muscle", upper),    # biceps
        bump(SHOULDER + ud * 15.0 + uv * 3.4, (3.4, 2.6, 7.0), 0.7, "muscle", upper),    # triceps
        bump(ELBOW + fd * 6.5 + fu * 1.8, (2.6, 3.2, 6.0), 0.6, "muscle", fore),          # forearm
        bump(ELBOW + uv * 4.3, (1.4, 1.4, 1.4), 0.35),                                   # point of the elbow
        bump(ELBOW - uv * 3.6, (2.4, 1.4, 1.2), -0.35),                                  # inside of the elbow
        # legs
        bump((14.6, -2.0, 68.0), (3.4, 4.0, 11.0), 0.6, "muscle"),
        bump((10.4, -7.0, 70.0), (3.6, 3.0, 10.0), 0.5, "muscle"),
        bump((7.6, -4.5, 57.5), (3.0, 3.0, 4.2), 0.7, "muscle"),              # teardrop above the knee
        bump((10.6, 5.6, 71.0), (4.0, 3.0, 10.0), 0.4, "muscle"),             # hamstrings
        bump((11.0, -6.6, 51.5), (2.2, 1.6, 2.5), 0.45),                      # kneecap
        bump((11.0, 5.3, 50.0), (3.0, 2.0, 2.4), -0.45),                      # back of the knee
        # neck
        bump((4.4, -2.8, 155.5), (1.3, 1.3, 5.0), 0.35, "body", neck),        # neck tendon
        bump((0.0, -5.4, 154.3), (1.0, 1.2, 1.3), 0.55, "male"),              # Adam's apple
        # face
        bump(EYE + Vector((0.0, -0.85, 0.0)), (2.0, 2.0, 1.5), -0.95, "face"),   # eye sockets
        bump(EYE + Vector((0.05, -1.45, 1.35)), (1.9, 1.3, 0.75), 0.45, "face"),  # upper lid
        bump(EYE + Vector((0.05, -1.35, -1.35)), (1.7, 1.2, 0.5), 0.2, "face"),   # lower lid
        bump((3.4, -9.3, 171.9), (2.7, 1.6, 0.9), 0.45, "brow"),
        bump((0.0, -10.0, 166.3), (1.15, 2.0, 3.0), 1.7, "face"),             # nose
        bump((0.0, -10.8, 165.0), (1.25, 1.6, 1.15), 0.9, "face"),            # tip of the nose
        bump((1.4, -9.5, 164.7), (0.9, 1.4, 0.8), 0.55, "face"),              # nostril wings
        bump((0.0, -9.6, 161.9), (2.4, 1.5, 0.75), 0.5, "lips"),              # upper lip
        bump((0.0, -9.4, 160.7), (2.2, 1.5, 0.75), 0.55, "lips"),             # lower lip
        bump((0.0, -9.8, 161.3), (2.4, 1.5, 0.22), -0.4, "lips"),             # mouth line
        bump((2.6, -9.3, 161.3), (0.7, 1.4, 0.7), -0.25, "lips"),             # mouth corners
        bump((0.0, -9.0, 158.6), (2.2, 2.0, 1.5), 0.45, "jaw"),               # chin
        bump((5.6, -7.0, 166.8), (2.0, 2.4, 1.6), 0.45, "cheek"),
        bump((6.6, -3.0, 159.5), (1.8, 3.0, 1.8), 0.4, "jaw"),                # angle of the jaw
        bump((8.8, 1.0, 167.5), (1.4, 2.0, 3.0), 1.5, "face"),                # ear
        bump((9.6, 0.8, 167.6), (0.9, 1.2, 1.6), -0.6, "face"),               # ear bowl
        bump((0.0, 9.5, 166.0), (5.0, 3.0, 3.0), 0.4, "face"),                # back of the skull
    ]
    back, _ = PALM[2]
    for i in range(4):                                                        # knuckles
        features.append(bump((back[i] + back[i + 1]) / 2.0 + HU * 0.5, (0.9, 0.9, 0.9), 0.25))
    mirrored = []
    for center, radii, amp, group, axes in features:
        if abs(center.x) > 0.01:
            flipped = tuple(side_vector(a, -1.0) for a in axes) if axes else None
            mirrored.append((side_vector(center, -1.0), radii, amp, group, flipped))
    return features + mirrored


ARM_CHAIN = (SHOULDER, ELBOW, WRIST, WRIST + HD * 25.0)     # to beyond the fingertips
HAIRLINE = ([0, 30, 60, 90, 115, 140, 180], [174.8, 174.3, 172.3, 170.6, 164.0, 158.6, 157.4])   # degrees round the head, height


def regions(P):
    """Masks of the outfit and the hair for points of the neutral body in the rest pose."""
    ax, y, z = np.abs(P[:, 0]), P[:, 1], P[:, 2]
    Q = np.stack([ax, y, z], axis=1)
    best_t = np.zeros(len(P))
    best_d = np.full(len(P), 1e9)
    offset = 0.0
    for a, b in zip(ARM_CHAIN[:-1], ARM_CHAIN[1:]):
        A, B = np.array(a), np.array(b)
        AB = B - A
        length = float(np.linalg.norm(AB))
        t = np.clip((Q - A) @ AB / (length * length), 0.0, 1.0)
        dist = np.linalg.norm(Q - (A + t[:, None] * AB), axis=1)
        better = dist < best_d
        best_t = np.where(better, offset + t * length, best_t)
        best_d = np.minimum(best_d, dist)
        offset += length
    arm = (best_d < 13.0) & (ax > 16.0) & (z > 60.0)     # 13 cm: the thumb tip is ~10 cm off the axis
    psi = np.degrees(np.arctan2(ax, -(y - 0.8)))
    hairline = np.interp(psi, *HAIRLINE)
    ear = (ax > 8.3) & (np.abs(y - 1.0) < 2.6) & (z > 163.5) & (z < 171.5)
    hair = np.clip((z - hairline) / 0.8 + 0.25, 0.0, 1.0) * (z > 156.0) * ~ear * ~arm
    collar = 148.6 + 2.0 * psi / 180.0
    shirt = (~arm & (z >= 104.2) & ((z < collar) | ((ax > 6.8) & (z < 151.5)))) | (arm & (best_t < 12.8))
    belt = ~arm & (z >= 100.8) & (z < 104.2)
    trousers = ~arm & (z < 100.8) & (z >= 27.0)
    boots = ~arm & (z < 27.0)
    return dict(arm=arm, t=best_t, hair=hair, collar=collar, shirt=shirt, belt=belt, trousers=trousers, boots=boots)


def outfit_offset(P):
    r = regions(P)
    z, ax = P[:, 2], np.abs(P[:, 0])
    offset = 0.15 * r["shirt"] + 0.45 * r["belt"] + 0.12 * r["trousers"] + 0.35 * r["boots"]
    offset += 0.3 * (r["arm"] & (r["t"] > 11.8) & (r["t"] < 12.8))                       # sleeve hem
    offset += 0.25 * (r["shirt"] & ~r["arm"] & (z > r["collar"] - 0.9) & (ax < 7.5))      # collar
    offset += 0.35 * (r["boots"] & (z > 24.5))                                            # cuff of the boot
    offset += r["hair"] * (1.0 + 0.7 * np_smooth(z, 174.0, 181.0))
    return offset


def sculpt(obj, preset):
    mesh = obj.data
    n = len(mesh.vertices)
    co = np.empty(n * 3)
    mesh.vertices.foreach_get("co", co)
    co = co.reshape(n, 3)
    normals = np.array([v.normal[:] for v in mesh.vertices])
    gains = PRESETS[preset].get("gain", {})
    offset = np.zeros(n)
    for center, radii, amp, group, axes in sculpt_features():
        gain = gains.get(group, 0.0 if group in ("male", "bust") else 1.0)
        if gain == 0.0:
            continue
        rel = co - np.array(center)
        if axes:
            rel = rel @ np.array([list(a) for a in axes]).T
        d2 = ((rel / np.array(radii)) ** 2).sum(axis=1)
        offset += amp * gain * np.clip(1.0 - d2, 0.0, 1.0) ** 2
    offset += outfit_offset(co)
    co += normals * offset[:, None]
    # Feet on the floor: the lowest point at 0, the soles flat.
    co[:, 2] -= co[:, 2].min()
    low = co[:, 2] < 0.8
    co[low, 2] *= 0.2
    mesh.vertices.foreach_set("co", co.ravel())
    mesh.update()


def add_eyes(obj):
    """Two quad spheres, weighted to the head, joined into the body."""
    verts, faces = [], []
    for sign in (1.0, -1.0):
        bm = bmesh.new()
        bmesh.ops.create_cube(bm, size=2.0)
        bmesh.ops.subdivide_edges(bm, edges=bm.edges[:], cuts=7, use_grid_fill=True)
        centre = side_vector(EYE, sign)
        start = len(verts)
        bm.verts.index_update()
        verts += [tuple(centre + v.co.normalized() * EYE_RADIUS) for v in bm.verts]
        faces += [tuple(start + v.index for v in f.verts) for f in bm.faces]
        bm.free()
    eyes = hb.new_mesh_object("eyes", verts, faces)
    eyes.vertex_groups.new(name="head").add(list(range(len(verts))), 1.0, 'REPLACE')
    hb.deselect_all()
    eyes.select_set(True)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.join()


def build_body(preset, name):
    """The cage of a preset as a subdivided, sculpted mesh object with vertex groups (weights)."""
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
    sculpt(obj, preset)
    add_eyes(obj)
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
    bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.004)
    bpy.ops.object.mode_set(mode='OBJECT')
    return obj


def build_shape_keys(obj):
    """The presets have the same topology, so their vertices are the shape keys."""
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


# --- Texture --------------------------------------------------------------------------------
SKIN = (0.80, 0.58, 0.45)
SKIN_WARM = (0.84, 0.52, 0.44)
HAIR = (0.17, 0.11, 0.07)
LIPS = (0.66, 0.38, 0.34)
SHIRT = (0.22, 0.40, 0.44)
TROUSERS = (0.28, 0.24, 0.20)
BELT = (0.13, 0.09, 0.06)
BUCKLE = (0.66, 0.58, 0.40)
BOOTS = (0.34, 0.22, 0.13)
SOLE = (0.10, 0.08, 0.07)
EYE_WHITE = (0.93, 0.92, 0.90)
IRIS = (0.24, 0.42, 0.56)
PUPIL = (0.03, 0.03, 0.04)


def texture_colors(P):
    """sRGB colour and tint mask for object positions P (N x 3) of the neutral body."""
    r = regions(P)
    x, y, z = P[:, 0], P[:, 1], P[:, 2]
    ax = np.abs(x)
    col = np.tile(np.array(SKIN), (len(P), 1))

    def paint(mask, color, amount=1.0):
        a = (np.asarray(mask, dtype=float) * amount)[:, None]
        col[:] = col * (1.0 - a) + np.array(color) * a

    face = (y < -5.0) & (z > 156.0) & (z < 176.0)
    paint(face & (ax > 4.0) & (ax < 7.5) & (np.abs(z - 166.2) < 2.0), SKIN_WARM, 0.35)      # cheeks
    paint(face & (ax < 1.6) & (z > 163.8) & (z < 167.0) & (y < -10.0), SKIN_WARM, 0.4)      # tip of the nose
    lips = face & (((ax / 2.4) ** 2 + ((z - 161.3) / 0.95) ** 2) < 1.0) & (y < -8.0)
    paint(lips, LIPS)
    paint(lips & (np.abs(z - 161.3) < 0.1) & (ax < 2.2), (0.30, 0.15, 0.13))               # mouth line
    s = np.clip((ax - 1.2) / 4.4, 0.0, 1.0)
    brow = face & (ax > 1.2) & (ax < 5.6) & (np.abs(z - (171.6 + 0.6 * np.sin(np.pi * s))) < 0.34 - 0.14 * s)
    paint(brow, HAIR)
    fabric = 1.0 + 0.035 * np.sin(x * 1.9 + z * 0.7) * np.sin(y * 2.3 - z * 1.3)
    paint(r["trousers"], TROUSERS)
    paint(r["boots"], BOOTS)
    paint(r["boots"] & (z > 24.5), (0.42, 0.29, 0.18))                                      # cuff
    paint(r["boots"] & (z < 1.3), SOLE)
    paint(r["shirt"], SHIRT)
    paint(r["shirt"] & r["arm"] & (r["t"] > 11.8), (0.16, 0.30, 0.33))                     # sleeve hem
    paint(r["shirt"] & ~r["arm"] & (z > r["collar"] - 0.9) & (ax < 7.5), (0.16, 0.30, 0.33))   # collar
    paint(r["belt"], BELT)
    paint(r["belt"] & (ax < 2.3) & (y < -7.0) & (z > 101.3) & (z < 103.7), BUCKLE)
    clothes = r["shirt"] | r["trousers"] | r["boots"]
    col[clothes] *= fabric[clothes, None]
    paint(r["hair"], HAIR)
    col[r["hair"] > 0.5] *= (1.0 + 0.08 * np.sin(z * 9.0 + x * 3.0))[r["hair"] > 0.5, None]   # strands
    for sign in (1.0, -1.0):
        centre = np.array(side_vector(EYE, sign))
        rel = P - centre
        eye = np.linalg.norm(rel, axis=1) < EYE_RADIUS + 0.08
        flat = np.hypot(rel[:, 0], rel[:, 2])
        front = rel[:, 1] < -0.6
        dist = np.linalg.norm(rel, axis=1)
        lash = ~eye & (dist < EYE_RADIUS + 0.45) & (rel[:, 2] > 0.25) & (rel[:, 1] < -0.2)
        paint(lash, (0.12, 0.08, 0.06), 0.85)                                               # lash line
        paint(eye, EYE_WHITE)
        paint(eye & front & (flat < 0.72), IRIS)
        paint(eye & front & (flat < 0.72) & (flat > 0.6), (0.10, 0.18, 0.25))
        paint(eye & front & (flat < 0.3), PUPIL)
    tint = r["shirt"].astype(float)
    return np.clip(col, 0.0, 1.0), tint


def build_material(obj):
    """Bakes the object position of every texel, computes the colours from it, saves the PNG."""
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 1
    material = hb.new_material("M_" + NAME)
    nodes, links = material.node_tree.nodes, material.node_tree.links
    output = next(n for n in nodes if n.type == 'OUTPUT_MATERIAL')
    coords = nodes.new('ShaderNodeTexCoord')
    encode = nodes.new('ShaderNodeVectorMath')
    encode.operation = 'MULTIPLY_ADD'
    encode.inputs[1].default_value = (1.0 / 400.0,) * 3
    encode.inputs[2].default_value = (0.5,) * 3
    links.new(coords.outputs['Object'], encode.inputs[0])
    emission = nodes.new('ShaderNodeEmission')
    links.new(encode.outputs['Vector'], emission.inputs['Color'])
    links.new(emission.outputs['Emission'], output.inputs['Surface'])
    size = TEXTURE_SIZE
    positions = bpy.data.images.new("positions", size, size, alpha=True, float_buffer=True)
    positions.pixels.foreach_set(np.zeros(size * size * 4, dtype=np.float32))
    target = nodes.new('ShaderNodeTexImage')
    target.image = positions
    nodes.active = target
    obj.data.materials.append(material)
    hb.set_active(obj)
    bpy.ops.object.bake(type='EMIT', margin=6)
    pixels = np.empty(size * size * 4, dtype=np.float32)
    positions.pixels.foreach_get(pixels)
    pixels = pixels.reshape(-1, 4)
    covered = pixels[:, 3] > 0.5
    P = (pixels[covered, :3].astype(np.float64) - 0.5) * 400.0
    colors, tint = texture_colors(P)
    result = np.zeros((size * size, 4), dtype=np.float32)
    result[:, :3] = SKIN
    result[covered, :3] = colors
    result[covered, 3] = tint
    image = bpy.data.images.new("T_%s_color" % NAME, size, size, alpha=True)
    image.pixels.foreach_set(result.ravel())
    path = os.path.join(hb.ART_DIR, NAME, "T_%s_color.png" % NAME)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    image.filepath_raw = path
    image.file_format = 'PNG'
    image.save()
    bpy.data.images.remove(positions)
    for node in (coords, encode, emission, target):
        nodes.remove(node)
    # Alpha is the tint mask, not transparency: keep colour and alpha independent.
    image.alpha_mode = 'CHANNEL_PACKED'
    texture = nodes.new('ShaderNodeTexImage')
    texture.image = image
    shader = nodes.new('ShaderNodeBsdfPrincipled')
    shader.inputs['Roughness'].default_value = 0.65
    links.new(texture.outputs['Color'], shader.inputs['Base Color'])
    links.new(shader.outputs['BSDF'], output.inputs['Surface'])
    print("build_humanoid: texture " + path)


# --- Rig ------------------------------------------------------------------------------------

def socket_bones():
    """name -> (parent, head, tail, roll target) for what the hands hold (right hand, left forearm)."""
    d, u, _ = arm_frame(FOREARM_DIR)
    grip_r = side_vector(GRIP, -1.0)
    blade_r = side_vector(-HV, -1.0)                       # forwards, out of the fist above the thumb
    strap = ELBOW + d * 13.0 + u * 4.4                     # on the outside of the forearm
    return {
        "weapon_r": ("hand_r", grip_r, grip_r + blade_r * 12.0, side_vector(HU, -1.0)),
        "shield_l": ("lowerarm_l", strap, strap + u * 10.0, -d),
    }


def bone_list():
    front, up = Vector((0.0, -1.0, 0.0)), Vector((0.0, 0.0, 1.0))
    bones = [   # name, parent, head, tail, direction the bone's Z axis looks to (roll)
        ("root", None, Vector((0, 0, 0)), Vector((0, 8, 0)), up),
        ("pelvis", "root", Vector((0, 0.6, 97)), Vector((0, 0.6, 104)), front),
        ("spine_01", "pelvis", Vector((0, 0.6, 104)), Vector((0, 0.2, 117.5)), front),
        ("spine_02", "spine_01", Vector((0, 0.2, 117.5)), Vector((0, 0, 131)), front),
        ("spine_03", "spine_02", Vector((0, 0, 131)), Vector((0, 0.6, 151.5)), front),
        ("neck_01", "spine_03", Vector((0, 0.6, 151.5)), Vector((0, 0.6, 158)), front),
        ("head", "neck_01", Vector((0, 0.6, 158)), Vector((0, 0.6, 179)), front),
    ]
    for suffix, sign in (("_l", 1.0), ("_r", -1.0)):
        p = lambda v: side_vector(v, sign)   # noqa: E731
        back_of_hand = p(HU)
        bones += [
            ("clavicle" + suffix, "spine_03", p(Vector((2.5, -1.0, 149.0))), p(SHOULDER), front),
            ("upperarm" + suffix, "clavicle" + suffix, p(SHOULDER), p(ELBOW), front),
            ("lowerarm" + suffix, "upperarm" + suffix, p(ELBOW), p(WRIST), front),
            ("hand" + suffix, "lowerarm" + suffix, p(WRIST), p(WRIST + HD * 9.5), back_of_hand),
            ("thigh" + suffix, "pelvis", p(HIP), p(KNEE), front),
            ("calf" + suffix, "thigh" + suffix, p(KNEE), p(ANKLE), front),
            ("foot" + suffix, "calf" + suffix, p(ANKLE), p(BALL), up),
            ("ball" + suffix, "foot" + suffix, p(BALL), p(TOE), up),
        ]
        joints = thumb_joints()
        for n in range(3):
            parent = "hand" + suffix if n == 0 else "thumb_0%d%s" % (n, suffix)
            bones.append(("thumb_0%d%s" % (n + 1, suffix), parent, p(joints[n]), p(joints[n + 1]), back_of_hand))
        for i in range(4):
            name, joints = finger_joints(i)
            for n in range(3):
                parent = "hand" + suffix if n == 0 else "%s_0%d%s" % (name, n, suffix)
                bones.append(("%s_0%d%s" % (name, n + 1, suffix), parent, p(joints[n]), p(joints[n + 1]), back_of_hand))
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


# --- Poses ----------------------------------------------------------------------------------
# A pose is {bone: [(axis, degrees), ...]}: turns around the world axes, on top of what the
# parent bone does, in the order given.
#   "x": forward (+) / back (-): a hanging limb swings forward, the trunk and head bow forward;
#   "y": a hanging limb goes to the character's right (+) / left (-), so +y lifts the right
#        arm sideways and lowers the left one;
#   "z": turn to the character's left (+) / right (-);
#   "curl": a finger or thumb bends towards the palm (around its own X axis).
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
            if axis == "curl":
                rotation = rotation @ Matrix.Rotation(math.radians(-degrees), 3, 'X')
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
    for suffix, sign in (("_l", 1.0), ("_r", -1.0)):
        for bone, point in SOLE_POINTS:
            pbone = arm.pose.bones[bone + suffix]
            heights.append((pbone.matrix @ pbone.bone.matrix_local.inverted() @ side_vector(point, sign)).z)
    return min(heights)


def fists(amount=1.0):
    """Fingers closed round a grip (both hands)."""
    pose = {}
    for suffix in ("_l", "_r"):
        for name, _, _ in FINGERS:
            for n, degrees in ((1, 78), (2, 88), (3, 55)):
                pose["%s_0%d%s" % (name, n, suffix)] = [("curl", degrees * amount)]
        for n, degrees in ((1, 18), (2, 30), (3, 35)):
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

def report(obj, arm):
    mesh = obj.data
    quads = sum(1 for p in mesh.polygons if len(p.vertices) == 4)
    triangles = sum(1 for p in mesh.polygons if len(p.vertices) == 3)
    bm = bmesh.new()
    bm.from_mesh(mesh)
    open_edges = sum(1 for e in bm.edges if not e.is_manifold)
    poles = sum(1 for v in bm.verts if len(v.link_edges) > 5)
    bm.free()
    tree = mathutils.kdtree.KDTree(len(mesh.vertices))
    for v in mesh.vertices:
        tree.insert(v.co, v.index)
    tree.balance()
    lopsided = sum(1 for v in mesh.vertices if tree.find((-v.co.x, v.co.y, v.co.z))[2] > 0.01)
    influences = max(len(v.groups) for v in mesh.vertices)
    size = obj.dimensions
    print("build_humanoid: size %.0f x %.0f x %.1f cm, %d vertices, %d faces (%d quads, %d triangles, %d n-gons, %d triangles in the game), "
          "%d open edges, %d vertices with more than 5 edges, %d vertices without a mirror partner, at most %d bones per vertex"
          % (size.x, size.y, size.z, len(mesh.vertices), len(mesh.polygons), quads, triangles, len(mesh.polygons) - quads - triangles,
             2 * quads + triangles, open_edges, poles, lopsided, influences))
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


def move_legs(arm, factor):
    """The other way to shorten the legs: calf and foot moved along their offsets, nothing
    scaled (what a pose with changed bone translations does). Rest pose only."""
    for side in ("_l", "_r"):
        thigh, calf, foot = (arm.data.bones[name + side] for name in ("thigh", "calf", "foot"))
        knee = thigh.head_local + (calf.head_local - thigh.head_local) * factor
        ankle = knee + (foot.head_local - calf.head_local) * factor
        for bone, head in ((calf, knee), (foot, ankle)):
            arm.pose.bones[bone.name].matrix = Matrix.Translation(head) @ bone.matrix_local.to_3x3().to_4x4()
            bpy.context.view_layer.update()
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

    # The three presets in the rest pose, without the props; faces close up.
    show_props(False)
    apply_pose(arm, {})
    for preset in ("neutral",) + PRESET_KEYS:
        set_preset(preset)
        hb.render_preview("%s_rest_%s" % (NAME, preset), (0, 0, 92), 330, size=(600, 800), views=turnaround)
        hb.render_preview("%s_face_%s" % (NAME, preset), (0, -2, 167), 62, size=(600, 600),
                          views=(("front", 0.0), ("front_left", -35.0), ("side", -90.0)))
    set_preset("neutral")
    hand = WRIST + HD * 7.0
    hb.render_preview("%s_hand_open" % NAME, tuple(hand), 48, size=(600, 600), views=(("front_left", -35.0), ("side", -90.0)))
    apply_pose(arm, fists())
    hb.render_preview("%s_hand_fist" % NAME, tuple(hand), 48, size=(600, 600), views=(("front_left", -35.0), ("side", -90.0)))
    show_props(True)
    hb.render_preview("%s_hand_grip" % NAME, tuple(side_vector(hand, -1.0)), 60, size=(600, 600), views=(("front_right", 35.0), ("side_right", 90.0)))
    show_props(False)

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
    move_legs(arm, 0.8)
    hb.render_preview("%s_rest_legs80_moved" % NAME, (0, 0, 92), 330, size=(600, 800), views=(("front", 0.0), ("front_left", -35.0)))
    arm.location.z = 0.0
    apply_pose(arm, {})

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
    build_material(obj)
    build_shape_keys(obj)
    arm = build_rig(obj)
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
