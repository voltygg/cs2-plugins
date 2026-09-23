"""The files the compiler reads: .vmat materials and ModelDoc .vmdl models. Needs no Blender."""

HEADER = (
    "<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} "
    "format:modeldoc28:version{fb63b6ca-f435-4aa0-a2c7-c66ddc651dca} -->\n"
)


def write_vmat(
    path, folder, color, rough, normal=None, metalness=0.6, roughness=0.55, glow_mask=None, glow=1.0
):
    """Writes a csgo_complex material; texture names are files in @p folder, e.g. "models/x/y/".

    @p rough is the addon path of a flat white roughness texture, scaled by @p roughness.
    With @p glow_mask the material glows at @p glow times its colour where the mask is white.
    """
    keys = [("shader", "csgo_complex.vfx")]
    if glow_mask:
        keys.append(("F_SELF_ILLUM", "1"))
    keys += [("g_flMetalness", f"{metalness:g}"), ("g_flRoughnessScaleFactor", f"{roughness:g}")]
    if glow_mask:
        keys.append(("g_flSelfIllumScale", f"{glow:g}"))
    keys += [
        ("g_vColorTint", "[1.000000 1.000000 1.000000 0.000000]"),
        ("TextureAmbientOcclusion", "materials/default/default_ao.tga"),
        ("TextureColor", folder + color),
        ("TextureNormal", folder + normal if normal else "materials/default/default_normal.tga"),
        ("TextureRoughness", rough),
    ]
    if glow_mask:
        keys.append(("TextureSelfIllumMask", folder + glow_mask))
    body = "".join(f'\t"{k}"\t"{v}"\n' for k, v in keys)
    with open(path, "w", newline="\n") as f:
        f.write(f'"Layer0"\n{{\n{body}}}\n')
    return path


def write_vmdl(path, meshes, hulls=(), animations=(), material_groups=None, attachments=()):
    """Writes a ModelDoc .vmdl.

    @p meshes: [(name, dmx)]. @p hulls: [dmx]. @p animations: [(name, dmx, looping)], the first
    playing when the prop spawns. @p material_groups: {group: {from vmat: to vmat}}, the first
    being the default. @p attachments: [(name, bone, (x, y, z), (pitch, yaw, roll))].
    """
    nodes = [
        {"_class": "BoneMarkupList", "children": [], "bone_cull_type": "None"},
        listing("RenderMeshList", [render_mesh(*m) for m in meshes]),
    ]
    if material_groups:
        groups = [material_group(i, *g) for i, g in enumerate(material_groups.items())]
        nodes.append(listing("MaterialGroupList", groups))
    if attachments:
        nodes.append(listing("AttachmentList", [attachment(*a) for a in attachments]))
    if animations:
        nodes.append(listing("AnimationList", [animation(*a) for a in animations]))
    if hulls:
        nodes.append(listing("PhysicsShapeList", [physics_hull(h) for h in hulls]))
    root = {"rootNode": {"_class": "RootNode", "children": nodes}}
    with open(path, "w", newline="\n") as f:
        f.write(HEADER + kv3(root) + "\n")
    return path


def listing(cls, children):
    return {"_class": cls, "children": children}


def render_mesh(name, dmx):
    return {"_class": "RenderMeshFile", "name": name, "filename": dmx}


def material_group(index, name, remaps):
    pairs = [{"_class": "BaseMaterialRemap", "from": a, "to": b} for a, b in remaps.items()]
    kind = "DefaultMaterialGroup" if index == 0 else "MaterialGroup"
    return {"_class": kind, "name": name, "remaps": pairs}


def attachment(name, bone, origin, angles):
    return {
        "_class": "Attachment",
        "name": name,
        "ignore_rotation": False,
        "parent_bone": bone,
        "relative_origin": list(origin),
        "relative_angles": list(angles),
        "weight": 1.0,
    }


def animation(name, dmx, looping):
    return {
        "_class": "AnimFile",
        "name": name,
        "source_filename": dmx,
        "fade_in_time": 0.0,
        "fade_out_time": 0.0 if looping else 0.1,
        "looping": looping,
        "delta": False,
        "worldSpace": False,
        "hidden": False,
    }


def physics_hull(dmx):
    return {
        "_class": "PhysicsHullFile",
        "filename": dmx,
        "parent_bone": "",
        "surface_prop": "metal",
        "collision_tags": "",
        "name": "",
    }


def kv3(value, depth=0):
    pad = "\t" * depth
    if isinstance(value, dict):
        fields = "".join(f"{pad}\t{k} = {kv3(v, depth + 1)}\n" for k, v in value.items())
        return "{\n" + fields + pad + "}"
    if isinstance(value, list):
        if not value:
            return "[  ]"
        if all(isinstance(v, (int, float)) and not isinstance(v, bool) for v in value):
            return "[ " + ", ".join(kv3(v) for v in value) + " ]"
        return "[\n" + "".join(f"{pad}\t{kv3(v, depth + 1)},\n" for v in value) + pad + "]"
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, float):
        text = f"{value:.6f}".rstrip("0")
        return text + "0" if text.endswith(".") else text
    if isinstance(value, int):
        return str(value)
    return f'"{value}"'
