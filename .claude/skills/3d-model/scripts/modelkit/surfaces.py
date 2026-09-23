"""Textures and the Blender materials that preview them."""

import os

import bpy
import numpy as np

SIZE = 1024


def load_pixels(path, size=SIZE):
    """RGBA float pixels of the image at @p path, scaled to @p size square. Rows run bottom-up."""
    img = bpy.data.images.load(path, check_existing=False)
    if size and tuple(img.size) != (size, size):
        img.scale(size, size)
    px = np.array(img.pixels[:], dtype=np.float32).reshape(img.size[1], img.size[0], 4)
    bpy.data.images.remove(img)
    return px


def save_pixels(path, px, data=False):
    """Writes @p px as a PNG to @p path and returns it as an image; @p data marks a normal map."""
    name = os.path.basename(path)
    if img := bpy.data.images.get(name):
        bpy.data.images.remove(img)
    height, width = px.shape[:2]
    img = bpy.data.images.new(name, width, height, alpha=False, is_data=data)
    img.pixels[:] = px.ravel()
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    if data:
        img.colorspace_settings.name = "Non-Color"
    return img


def image(path, data=False):
    """Loads an existing PNG for a preview material."""
    img = bpy.data.images.load(path, check_existing=True)
    if data:
        img.colorspace_settings.name = "Non-Color"
    return img


def normal_from(px, strength):
    """A tileable normal map, height from brightness. Green points up the texture (OpenGL).

    Around 1.5 suits painted metal; 10 suits raised patterns such as tread plate.
    """
    height = px[..., :3] @ np.array([0.299, 0.587, 0.114], dtype=np.float32)
    dx = np.roll(height, -1, 1) - np.roll(height, 1, 1)
    dy = np.roll(height, -1, 0) - np.roll(height, 1, 0)
    n = np.dstack((-dx * strength, -dy * strength, np.ones_like(height)))
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    out = np.ones((*height.shape, 4), dtype=np.float32)
    out[..., :3] = n * 0.5 + 0.5
    return out


def brightened(px, factor):
    out = px.copy()
    out[..., :3] = np.clip(out[..., :3] * factor, 0, 1)
    return out


def solid(path, rgb, size=16):
    """A flat colour texture, for glow colours and self-illum masks."""
    return save_pixels(path, np.tile(np.array([*rgb, 1], dtype=np.float32), (size, size, 1)))


def replace_texture(source, color_path, normal_path=None, strength=1.5):
    """Installs a generated texture as @p color_path, with a matching normal map when asked.

    Keeping the file names keeps every .vmat valid; open previews pick the change up.
    """
    px = load_pixels(source)
    save_pixels(color_path, px)
    if normal_path:
        save_pixels(normal_path, normal_from(px, strength), data=True)
    reload_images()
    return px


def reload_images():
    for img in bpy.data.images:
        if img.filepath:
            img.reload()


def material(name, color=None, normal=None, metallic=0.7, roughness=0.5, glow=None):
    """A Blender material named like the game's, e.g. "jump_pad_body.vmat".

    The DMX records only the name; the game look comes from the .vmat.
    """
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    for node in list(nodes):
        if node.type not in ("BSDF_PRINCIPLED", "OUTPUT_MATERIAL"):
            nodes.remove(node)
    bsdf = nodes["Principled BSDF"]
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Emission Strength"].default_value = 0.0
    if color is not None:
        tex = nodes.new("ShaderNodeTexImage")
        tex.image = color
        links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    if normal is not None:
        tex = nodes.new("ShaderNodeTexImage")
        tex.image = normal
        normal_map = nodes.new("ShaderNodeNormalMap")
        links.new(tex.outputs["Color"], normal_map.inputs["Color"])
        links.new(normal_map.outputs["Normal"], bsdf.inputs["Normal"])
    if glow is not None:
        bsdf.inputs["Base Color"].default_value = (*glow, 1)
        bsdf.inputs["Emission Color"].default_value = (*glow, 1)
        bsdf.inputs["Emission Strength"].default_value = 1.2
    return mat
