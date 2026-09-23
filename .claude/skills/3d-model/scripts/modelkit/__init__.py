"""Blender helpers for Source 2 props. Load inside Blender, through execute_blender_code:

    import sys
    sys.path.insert(0, r"<repo>/.claude/skills/3d-model/scripts")
    import modelkit
    modelkit.reload()
    from modelkit import session, surfaces, shapes, objects, rig, preview, source2, modeldoc

One Blender unit is one Source unit; +X is the model's forward, +Z is up.
"""

import importlib
import sys

# Dependencies first, so a reload picks up edits in the modules the others import.
MODULES = ("session", "surfaces", "shapes", "objects", "rig", "preview", "source2", "modeldoc")


def reload():
    """Picks up edits to the kit without restarting Blender."""
    for name in MODULES:
        if module := sys.modules.get(f"{__name__}.{name}"):
            importlib.reload(module)
