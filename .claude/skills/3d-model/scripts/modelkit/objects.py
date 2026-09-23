"""Scene objects: finishing parts, joining them into the model, and editing a joined mesh."""

import math

import bmesh
import bpy
from mathutils import Vector


def to_object(name, bm):
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    ob = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(ob)
    return ob


def apply_modifiers(ob):
    bpy.ops.object.select_all(action="DESELECT")
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    for mod in list(ob.modifiers):
        bpy.ops.object.modifier_apply(modifier=mod.name)


def cut(ob, cutter_bm):
    """Subtracts `cutter_bm` from `ob`, e.g. vent slots."""
    cutter = to_object(ob.name + "_cutter", cutter_bm)
    mod = ob.modifiers.new("cut", "BOOLEAN")
    mod.operation = "DIFFERENCE"
    mod.solver = "EXACT"
    mod.object = cutter
    apply_modifiers(ob)
    bpy.data.objects.remove(cutter, do_unlink=True)
    return ob


def box_uv(bm, size):
    """Projects each face along its main axis; `size` is world units per texture repeat."""
    uv = bm.loops.layers.uv.verify()
    for face in bm.faces:
        n = face.normal
        axis = max(range(3), key=lambda i: abs(n[i]))
        a, b = [(1, 2), (0, 2), (0, 1)][axis]
        for loop in face.loops:
            loop[uv].uv = (loop.vert.co[a] / size, loop.vert.co[b] / size)


def finish(ob, mat, bone, uv_size=48.0, bevel=0.0):
    """Bevels hard edges, projects UVs, sets the material and binds every vertex to `bone`."""
    if bevel:
        mod = ob.modifiers.new("bevel", "BEVEL")
        mod.width = bevel
        mod.segments = 2
        mod.limit_method = "ANGLE"
        mod.angle_limit = math.radians(40)
        apply_modifiers(ob)
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    box_uv(bm, uv_size)
    bm.to_mesh(ob.data)
    bm.free()
    ob.data.materials.clear()
    ob.data.materials.append(mat)
    ob.vertex_groups.new(name=bone).add(range(len(ob.data.vertices)), 1.0, "REPLACE")
    return ob


def bind_along(ob, along, bottom, top):
    """Rebinds a finished spring so each vertex blends from `bottom` to `top` along the coil."""
    ob.vertex_groups.clear()
    low = ob.vertex_groups.new(name=bottom)
    high = ob.vertex_groups.new(name=top)
    for index, t in enumerate(along):
        low.add([index], 1.0 - t, "REPLACE")
        high.add([index], t, "REPLACE")
    return ob


def join(parts, name, smooth_degrees=35):
    """Joins `parts` into the first of them, renamed `name` and smooth-shaded."""
    bpy.ops.object.select_all(action="DESELECT")
    for ob in parts:
        ob.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    body = bpy.context.active_object
    body.name = name
    body.data.name = name
    bpy.ops.object.shade_smooth_by_angle(angle=math.radians(smooth_degrees))
    return body


def hull(name, bm, mat):
    """A static physics hull; hidden in renders, exported as its own DMX."""
    ob = finish(to_object(name, bm), mat, "root")
    ob.hide_render = True
    ob.display_type = "WIRE"
    return ob


def vertices(ob, material=None, bone=None):
    """Indices of `ob`'s vertices on faces with `material` and/or bound to `bone`."""
    chosen = set(range(len(ob.data.vertices)))
    if material is not None:
        slot = ob.data.materials.find(material)
        chosen &= {v for p in ob.data.polygons if p.material_index == slot for v in p.vertices}
    if bone is not None:
        group = ob.vertex_groups[bone].index
        chosen &= {
            v.index
            for v in ob.data.vertices
            if any(g.group == group and g.weight > 0 for g in v.groups)
        }
    return sorted(chosen)


def transform(ob, indices, move=(0, 0, 0), scale=1.0, pivot=(0, 0, 0)):
    """Scales the chosen vertices about `pivot`, then moves them; for editing a joined mesh."""
    pivot = Vector(pivot)
    for index in indices:
        vert = ob.data.vertices[index]
        vert.co = pivot + (vert.co - pivot) * scale + Vector(move)
    ob.data.update()


def replace_material(ob, old, new):
    """Points the faces using material `old` at `new`, e.g. a renamed .vmat."""
    ob.data.materials[ob.data.materials.find(old)] = new
