"""The Stronghold jump pad built with modelkit: a skinned, animated floor prop.

Rebuilds plugins/stronghold/addon/models/stronghold/jump_pad in the scene "jump_pad" from the
committed textures. Run it inside Blender with the repo root as REPO:

    ns = {"REPO": r"<repo>"}
    exec(open(r"<repo>/.claude/skills/3d-model/examples/jump_pad.py").read(), ns)
"""

import os
import sys

sys.path.insert(0, os.path.join(REPO, ".claude", "skills", "3d-model", "scripts"))  # noqa: F821
import modelkit  # noqa: E402

modelkit.reload()
from modelkit import objects, preview, rig, session, shapes, source2, surfaces  # noqa: E402

FOLDER = "models/stronghold/jump_pad/"
DIR = os.path.join(REPO, "plugins", "stronghold", "addon", FOLDER)  # noqa: F821
GLOW_CT = (0.20, 0.55, 1.00)

session.scene("jump_pad", FOLDER)


def texture(name, data=False):
    return surfaces.image(os.path.join(DIR, name), data)


metal_normal = texture("jump_pad_body_normal.png", data=True)
body_color = texture("jump_pad_body_color.png")
steel_color = texture("jump_pad_steel_color.png")
tread_color = texture("jump_pad_tread_color.png")
tread_normal = texture("jump_pad_tread_normal.png", data=True)
BODY = surfaces.material("jump_pad_body.vmat", body_color, metal_normal, 0.6, 0.55)
STEEL = surfaces.material("jump_pad_steel.vmat", steel_color, metal_normal, 0.9, 0.4)
TREAD = surfaces.material("jump_pad_tread.vmat", tread_color, tread_normal, 0.8, 0.5)
GLOW = surfaces.material("jump_pad_glow_blue.vmat", metallic=0.0, roughness=0.4, glow=GLOW_CT)


def part(name, bm, mat, bone, **finish):
    return objects.finish(objects.to_object(name, bm), mat, bone, **finish)


parts = []

# Housing: a skirt with glowing vent slots, a lip, the recess floor and the piston's collar.
vents = [22.5 + 45 * k for k in range(8)]
skirt = objects.to_object("skirt", shapes.cone(22.0, 20.8, 0.0, 2.4))
objects.cut(skirt, shapes.merge(shapes.radial_box(22.0, a, (3.6, 3.2), 0.7, 1.7) for a in vents))
parts.append(objects.finish(skirt, BODY, "root", bevel=0.2))
slots = shapes.merge(shapes.radial_box(20.05, a, (0.5, 3.0), 0.75, 1.65) for a in vents)
parts.append(part("vent_glow", slots, GLOW, "root"))
parts.append(part("lip", shapes.tube(17.6, 20.8, 2.4, 3.8), BODY, "root", bevel=0.2))
floor = shapes.merge([shapes.cone(17.6, 17.6, 0.0, 0.8), shapes.tube(3.2, 4.4, 0.8, 2.0, seg=24)])
parts.append(part("floor", floor, BODY, "root"))

# Armour, bolts and the direction notch on the lip; the front (+X) is the launch direction.
armor = shapes.merge(shapes.sector(18.0, 20.7, a - 9, a + 9, 3.8, 4.5) for a in range(30, 360, 60))
parts.append(part("armor", armor, STEEL, "root", bevel=0.15))
parts.append(part("bolts", shapes.bolts(19.3, range(60, 360, 60), 0.55, 3.8, 4.15), STEEL, "root"))
notch = shapes.prism([(20.4, 0), (18.3, 1.5), (18.3, -1.5)], 3.8, 3.86)
parts.append(part("notch", notch, GLOW, "root"))

# Plate: body, steel frame with bolts around a recessed tread insert, two chevrons, the piston.
parts.append(part("plate", shapes.cone(17.2, 17.2, 1.6, 3.4), BODY, "plate", bevel=0.15))
parts.append(part("frame", shapes.tube(15.4, 17.2, 3.4, 3.75), STEEL, "plate", bevel=0.1))
parts.append(part("tread", shapes.cone(15.4, 15.4, 3.3, 3.55), TREAD, "plate", uv_size=28.0))
rivets = shapes.bolts(16.3, range(0, 360, 45), 0.35, 3.75, 3.95)
parts.append(part("frame_bolts", rivets, STEEL, "plate"))
chevrons = []
for x in (-3.0, 1.8):
    width, half, tip = 1.1, 4.5, x + 3.5
    left = [(tip, 0), (tip - width, 0), (x - width, half), (x, half)]
    right = [(tip, 0), (x, -half), (x - width, -half), (tip - width, 0)]
    chevrons += [shapes.prism(left, 3.55, 3.62), shapes.prism(right, 3.55, 3.62)]
parts.append(part("chevrons", shapes.merge(chevrons), GLOW, "plate"))
parts.append(part("piston", shapes.cone(2.8, 2.8, -20.0, 1.6, seg=24), STEEL, "plate"))

# The coil hides under the plate at rest and stretches with it.
coil_bm, along = shapes.spring(8.0, 0.4, 6, 1.0, 1.2)
parts.append(objects.bind_along(part("coil", coil_bm, STEEL, "root"), along, "root", "plate"))

body = objects.join(parts, "jump_pad")
bones = [("root", (0, 0, 0), (0, 0, 3), None), ("plate", (0, 0, 3), (0, 0, 6), "root")]
arm = rig.skeleton("jump_pad_skeleton", bones, [body])
objects.hull("jump_pad_hull", shapes.cone(22.0, 22.0, 0.0, 3.8, seg=16), BODY)


def plate_keys(keys):
    """(frame, lift, tilt) keys; a positive tilt raises the rear so the plate throws forward."""
    return {"plate": [(frame, (0, 0, lift), (0, tilt, 0)) for frame, lift, tilt in keys]}


rig.animate(arm, "idle", plate_keys([(0, 0, 0), (24, 1.6, 0), (48, 0, 0)]))
launch = [(0, 0, 0), (2, -1.2, 0), (4, 16.0, 24), (6, 17.0, 22), (10, 11.0, 12)]
launch += [(14, -0.8, -1.5), (17, 3.0, 3), (20, -0.6, -1), (23, 0.6, 0.3), (26, 0, 0)]
rig.animate(arm, "launch", plate_keys(launch))
rig.show(arm, "idle")
source2.remember(
    "jump_pad",
    model="jump_pad",
    armature="jump_pad_skeleton",
    actions=("idle", "launch"),
    files={"jump_pad": "jump_pad_jump_pad"},
)
preview.stage()

result = {
    "tris": sum(len(p.vertices) - 2 for p in body.data.polygons),
    "size": [round(v, 2) for v in body.dimensions],
}
