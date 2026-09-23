"""The skeleton and its actions."""

import math

import bpy
from mathutils import Euler, Matrix, Vector


def skeleton(name, bones, meshes):
    """Creates the armature @p name and skins @p meshes to it.

    @p bones is a list of (name, head, tail, parent or None), parents first. A mesh's vertex
    groups name the bones it follows.
    """
    data = bpy.data.armatures.get(name) or bpy.data.armatures.new(name)
    arm = bpy.data.objects.new(name, data)
    bpy.context.scene.collection.objects.link(arm)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.mode_set(mode="EDIT")
    for bone in list(data.edit_bones):
        data.edit_bones.remove(bone)
    for bone_name, head, tail, parent in bones:
        bone = data.edit_bones.new(bone_name)
        bone.head, bone.tail = head, tail
        if parent:
            bone.parent = data.edit_bones[parent]
    bpy.ops.object.mode_set(mode="OBJECT")
    for ob in meshes:
        ob.parent = arm
        ob.modifiers.new("skeleton", "ARMATURE").object = arm
    data.vs.action_selection = "CURRENT"
    return arm


def animate(arm, name, tracks):
    """Records the action @p name on @p arm, replacing one of that name, and returns it.

    @p tracks maps a bone to keys (frame, (dx, dy, dz), (rx, ry, rz) degrees): a move and a turn
    about the bone's head, in model axes, relative to rest and to the parent bone. Untracked bones
    stay at rest. A looping action ends on the key it starts with.
    """
    if action := bpy.data.actions.get(name):
        bpy.data.actions.remove(action)
    action = bpy.data.actions.new(name)
    action.use_fake_user = True
    arm.animation_data_create()
    arm.animation_data.action = action
    for bone_name, keys in tracks.items():
        pose = arm.pose.bones[bone_name]
        pose.rotation_mode = "QUATERNION"
        rest = pose.bone.matrix_local
        head = rest.to_translation()
        for frame, move, turn in keys:
            rotation = Euler([math.radians(v) for v in turn]).to_matrix().to_4x4()
            delta = Matrix.Translation(Vector(move) + head) @ rotation @ Matrix.Translation(-head)
            pose.matrix_basis = rest.inverted() @ delta @ rest
            for path in ("location", "rotation_quaternion", "scale"):
                pose.keyframe_insert(path, frame=frame)
    return action


def show(arm, action, frame=0):
    """Poses @p arm at @p frame of @p action."""
    arm.animation_data.action = bpy.data.actions[action]
    bpy.context.scene.frame_set(frame)
