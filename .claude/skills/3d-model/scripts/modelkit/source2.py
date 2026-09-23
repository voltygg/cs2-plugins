"""Exporting the scene: DMX files for the compiler, and the .blend for later edits."""

import os
import re
import shutil
import tempfile

import bpy

from . import rig

SETTINGS = "modelkit"


def remember(scene_name, model, armature=None, actions=(), files=None):
    """Stores how the scene exports, so a later session can re-export it from the .blend alone.

    `model` names the animation files (`<model>_<action>.dmx`); `actions` list the actions,
    the one to pose after export first; `files` renames mesh files ({object: stem}), which otherwise
    take the object's name.
    """
    bpy.data.scenes[scene_name][SETTINGS] = {
        "model": model,
        "armature": armature or "",
        "actions": list(actions),
        "files": dict(files or {}),
    }


def settings(scene_name):
    stored = bpy.data.scenes[scene_name].get(SETTINGS)
    if stored is None:
        raise ValueError(f"scene {scene_name} has no export settings; call remember() first")
    return stored.to_dict()


def export(scene_name, out_dir):
    """Exports the scene's meshes and each remembered action as DMX into `out_dir`.

    Blender Source Tools exports only the current action and leaves object references stale,
    so everything is looked up by name on each pass.
    """
    kept = settings(scene_name)
    armature, actions, files = kept["armature"], kept["actions"], kept["files"]
    renamed = [
        m.name
        for ob in bpy.data.scenes[scene_name].objects
        if ob.type == "MESH" and ob.vs.export
        for m in ob.data.materials
        if m and re.search(r"\.\d{3}$", m.name)
    ]
    if renamed:
        raise ValueError(f"materials renamed by Blender would reach the DMX: {renamed}")
    staging = tempfile.mkdtemp(prefix="modelkit_")
    written = []
    try:
        for action in actions or [None]:
            sc = bpy.data.scenes[scene_name]
            bpy.context.window.scene = sc
            sc.vs.export_path = staging
            if armature and action:
                arm = bpy.data.objects[armature]
                arm.data.vs.action_selection = "CURRENT"
                arm.animation_data.action = bpy.data.actions[action]
            sc.frame_set(0)
            bpy.ops.export_scene.smd(export_scene=True)
            if armature and action:
                target = os.path.join(out_dir, f"{kept['model']}_{action}.dmx")
                shutil.copy(os.path.join(staging, "anims", f"{armature}.dmx"), target)
                written.append(target)
        for name in os.listdir(staging):
            stem, ext = os.path.splitext(name)
            if ext == ".dmx" and stem != armature:
                target = os.path.join(out_dir, files.get(stem, stem) + ".dmx")
                shutil.copy(os.path.join(staging, name), target)
                written.append(target)
    finally:
        shutil.rmtree(staging, ignore_errors=True)
        bpy.data.scenes[scene_name].vs.export_path = "//"
        if armature and actions:
            rig.show(bpy.data.objects[armature], actions[0])
    return sorted(written)


def save_blend(scene_name, path):
    """Writes only `scene_name` and its remembered actions to `path`, image paths relative.

    Other scenes, such as imported references, stay out.
    """
    actions = settings(scene_name)["actions"]
    blocks = {bpy.data.scenes[scene_name], *(bpy.data.actions[a] for a in actions)}
    bpy.data.libraries.write(path, blocks, path_remap="RELATIVE_ALL", fake_user=True, compress=True)
    return path
