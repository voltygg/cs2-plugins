"""The Blender session: its state, the model's scene, opening existing models, and cleanup."""

import os

import addon_utils
import bpy

PREVIEW_OBJECTS = ("preview_camera", "preview_sun", "preview_ground")


def status():
    """What the task needs from Blender, and what the user has open."""
    tools = next((m for m in addon_utils.modules() if m.__name__ == "io_scene_valvesource"), None)
    return {
        "blender": bpy.app.version_string,
        "source_tools": tools.bl_info["version"] if tools else None,
        "source_tools_enabled": bool(tools) and addon_utils.check(tools.__name__)[1],
        "file": bpy.data.filepath or None,
        "unsaved_changes": bpy.data.is_dirty,
        "scenes": [s.name for s in bpy.data.scenes],
    }


def scene(name, material_path):
    """Makes @p name the active scene, emptied except for the preview rig, set up for DMX export.

    @p material_path is the model's folder in the addon, e.g. "models/stronghold/jump_pad/".
    """
    sc = bpy.data.scenes.get(name) or bpy.data.scenes.new(name)
    for ob in list(sc.objects):
        if ob.name not in PREVIEW_OBJECTS:
            bpy.data.objects.remove(ob, do_unlink=True)
    for mesh in [m for m in bpy.data.meshes if m.users == 0]:
        bpy.data.meshes.remove(mesh)
    bpy.context.window.scene = sc
    sc.unit_settings.scale_length = 1.0
    sc.render.fps = 30
    configure_export(sc, material_path)
    return sc


def configure_export(sc, material_path):
    # Older DMX versions make resourcecompiler run dmxconvert, which fails under Program Files.
    sc.vs.dmx_encoding = "9"
    sc.vs.dmx_format = "22"
    sc.vs.material_path = material_path
    sc.vs.export_path = "//"


def open_model(blend_path, scene_name, replace=False):
    """Appends @p scene_name and its actions from a model's .blend into the open session.

    The user's own file stays open. Blender renames incoming data whose name is taken (a
    material becoming "x.vmat.001" breaks the DMX), so a clash is an error unless @p replace,
    which first deletes the session's copies.
    """
    with bpy.data.libraries.load(blend_path) as (source, _):
        incoming = {
            "scenes": [scene_name],
            "actions": list(source.actions),
            "materials": list(source.materials),
            "images": list(source.images),
        }
    clashes = [
        (kind, n) for kind, names in incoming.items() for n in names if n in getattr(bpy.data, kind)
    ]
    if clashes and not replace:
        raise ValueError(f"already in the session: {clashes}; pass replace=True to reload")
    remove_scene(scene_name)
    for kind, name in clashes:
        if kind != "scenes" and (data := getattr(bpy.data, kind).get(name)):
            getattr(bpy.data, kind).remove(data)
    with bpy.data.libraries.load(blend_path, link=False) as (_, target):
        target.scenes = [scene_name]
        target.actions = incoming["actions"]
    # An action no armature holds would go in the next purge.
    for action in target.actions:
        action.use_fake_user = True
    sc = bpy.data.scenes[scene_name]
    folder = os.path.dirname(blend_path)
    for img in bpy.data.images:
        if img.filepath and not os.path.exists(bpy.path.abspath(img.filepath)):
            local = os.path.join(folder, os.path.basename(img.filepath))
            if os.path.exists(local):
                img.filepath = local
                img.reload()
    bpy.context.window.scene = sc
    return sc


def import_dmx(scene_name, material_path, meshes, animations=()):
    """Builds an editable scene from a model that has no .blend: its mesh DMX files, then its
    animation DMX files onto the imported armature. Check the result; hulls come in as meshes.
    """
    sc = scene(scene_name, material_path)
    for path in meshes:
        bpy.ops.import_scene.smd(filepath=path, createCollections=False, append="APPEND")
    armature = next((ob for ob in sc.objects if ob.type == "ARMATURE"), None)
    for path in animations:
        bpy.context.view_layer.objects.active = armature
        bpy.ops.import_scene.smd(filepath=path, createCollections=False, append="APPEND")
    return sc


def remove_scene(name):
    """Deletes a scene this task created, such as an imported reference, with its own objects."""
    sc = bpy.data.scenes.get(name)
    if sc is None:
        return
    for ob in list(sc.objects):
        if len(ob.users_scene) == 1:
            bpy.data.objects.remove(ob, do_unlink=True)
    bpy.data.scenes.remove(sc)


def purge():
    """Removes data nothing uses; actions with a fake user stay. Returns how many went."""
    collections = (bpy.data.meshes, bpy.data.images, bpy.data.materials, bpy.data.actions)
    before = sum(len(c) for c in collections)
    bpy.data.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)
    return before - sum(len(c) for c in collections)
