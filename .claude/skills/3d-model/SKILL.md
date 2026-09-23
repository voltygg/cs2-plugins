---
name: 3d-model
description: Make, change, retexture or animate a 3D prop for any plugin's workshop addon in Blender (through the Blender MCP), compile it with the CS2 Workshop Tools, wire it into the plugin, and clean up afterwards. Use for "make a model for X", "replace the stand-in model", "reskin", "new texture for X", "animate the X", "the model looks too big/plain", or any new or changed .vmdl.
---

# 3D models

Blender runs on the user's machine with the `blender` MCP and Blender Source Tools. Work in
their open session, show them the result, and export only once they approve.

- `scripts/check.py`: checks the tools outside Blender.
- `scripts/modelkit/`: the helpers you run inside Blender:
  - `session`: status, scenes, opening models, cleanup
  - `surfaces`: textures and preview materials
  - `shapes`: mesh parts
  - `objects`: finishing and joining parts, editing a joined mesh
  - `rig`: skeleton and actions
  - `preview`: renders and the user's viewport
  - `source2`: DMX export and the `.blend`
  - `modeldoc`: `.vmat` and `.vmdl` writers; these also run without Blender
- `scripts/compile.py`: compiles and installs one model folder.
- `examples/jump_pad.py`: a complete skinned, animated prop. Read it before building one.

One Blender unit is one Source unit. +X is the model's forward and +Z is up. A player is
32 × 32 × 72 units, with eyes at 64.

## 1. Check the tools

```bash
uv run python .claude/skills/3d-model/scripts/check.py <plugin>
```

In Blender, load the kit and read the session:

```python
import sys
sys.path.insert(0, r"<repo>/.claude/skills/3d-model/scripts")
import modelkit
modelkit.reload()
from modelkit import session, surfaces, shapes, objects, rig, preview, source2, modeldoc
result = session.status()
```

Stop and tell the user when something is missing:

- Blender or its MCP isn't answering.
- `source_tools_enabled` is false.
- `check.py` reports MISSING. The Codex lines only matter when new textures are needed, and a
  running CS2 only blocks installing.

When the user has unsaved changes, work only in the model's own scene.

## 2. Pick the path

| The model | Start with |
| --- | --- |
| New | step 3, then build it in a new scene with `session.scene(name, "models/<plugin>/<model>/")` |
| Has a `.blend` beside its `.vmdl` | `session.open_model(<blend>, <scene>)`, which brings its scene, actions and export settings into the session |
| Has DMX files only, such as a reference-pack stand-in | `session.import_dmx(scene, folder, meshes, animations)`. Check the result, then `source2.remember(...)` |
| Needs new textures only | step 4 alone, then step 8. No geometry work |

`open_model` refuses when the session already holds data with the same names, because Blender
would rename the incoming materials to `x.vmat.001` and break the DMX. Pass `replace=True` to
reload over your own earlier copy, never over the user's work.

## 3. Read the contract

Before you design, find everything the plugin assumes about the model:

- The model path. Grep the plugin for `.vmdl`; in Stronghold it is `src/Config/ItemAssets.cpp`.
- What fixes the size: the model scale, the placement box, trigger radii (the jump pad's
  `Launcher`), and offsets measured on the mesh, such as muzzles and part stacking (`PartAt`).
- Names the code uses: attachments, animations, material groups or skins (`SkinOf`), and bones.
- The plugin's assets doc (`docs/assets.md`): addon layout and licence status.

Keep every name the code uses, or change the code in the same task.

## 4. Textures

Before the first Codex run, list `~/.codex/generated_images/`, so cleanup can tell which folders
the runs added. Run Codex in the background while you work:

```bash
codex exec --skip-git-repo-check --sandbox workspace-write -C "<scratchpad>/tex" "Use the imagegen skill (built-in image_gen tool) to generate <name>.png: albedo texture for a low-poly CS2 prop; perfectly seamless and tileable on both axes; flat orthographic; evenly lit albedo only (no baked shadows, highlights, perspective or vignette); square 1024x1024; no text, logos or watermark. Subject: <material>. Copy the final image into the current directory as <name>.png."
```

- Look at each result. Brushed or streaked metal turns into wood grain on curved surfaces, so
  ask for powder-coated or speckled metal.
- `surfaces.replace_texture(generated, color_png, normal_png, strength)` installs a texture at
  1024 px under the existing file name, rebuilds its normal map and refreshes the previews.
  Keeping the name keeps every `.vmat` valid. Use a strength of about 1.5 for painted metal and
  10 for raised patterns; stronger turns metal into foam.
- `surfaces.brightened` makes a lighter trim variant, and `surfaces.solid` makes glow colours
  and self-illum masks.

## 5. Build or edit

Write the build as a scratchpad script and run it with `execute_blender_code`. Keep to the
model's scene, and put imported references in a scene of their own.

- **New parts:** build the geometry with the `shapes` functions and cut holes with
  `objects.cut`. `objects.finish` bevels, projects UVs, sets the material and binds the part to a
  bone. `objects.join` merges parts into the render mesh, and `objects.hull` makes the collision
  mesh.
- **Changing a finished mesh:** `objects.vertices(body, material=..., bone=...)` picks the
  vertices of one part, and `objects.transform` moves or scales them. Add new parts with
  `objects.join([body, *parts], body.name)`, and repoint a renamed material with
  `objects.replace_material`. If the whole model only needs resizing, prefer the plugin's scale.
- **Looks:**
  - Compare the size against a player; a floor prop wider than about 40 units looks oversized.
  - Primitives alone look like programmer art. Add bevels, trim in a second material, bolts,
    vents, a recessed insert and normal maps. About 5–12k triangles is fine.
  - Keep glow to thin lines, slots and small markers, at glow 1.0. Large bands read as flat colour.
  - Put team colour on its own glow material and remap it in a material group (`blue`/`red`).

## 6. Animate

`rig.skeleton` creates the bones and skins the mesh; each vertex group names the bone its part
follows. `rig.animate` records an action from per-bone keys `(frame, move, turn)`, at 30 fps,
relative to rest; re-recording replaces the action. `source2.remember` stores the model name,
armature, actions and file renames, so a later session exports from the `.blend` alone.

- Exaggerate. A 10-unit lift looked static in game. Crouch before the action, overshoot, then
  bounce to rest. Something that should look alive needs a moving `idle` loop.
- A `prop_dynamic` plays no animation until told, so the plugin must start the idle loop
  (step 9). A looping action ends on its first key.
- Check a few frames of each action in a `preview.contact_sheet`, looking for parts that clip.

## 7. Review with the user

- Stage the preview with `preview.stage()` and show the model with `preview.frame_viewport`.
- Render stills from a player's view with `preview.render` (about 64 units up, 60 to 80 away).
- Tell the user how to play an action: select the armature, pick it in the Action Editor, press Space.
- Iterate. Do not export, compile or install before they approve.

## 8. Export and compile

```python
source2.export(scene, model_dir)                       # DMX: meshes, hull, one file per action
source2.save_blend(scene, model_dir + "/<model>.blend")
modeldoc.write_vmat(...)   # only for new or changed materials
modeldoc.write_vmdl(...)   # only when meshes, animations, groups or attachments change
```

```bash
uv run python .claude/skills/3d-model/scripts/compile.py plugins/<plugin>/addon models/<plugin>/<model> --install client server
```

- `export` refuses renamed `.001` materials, and it leaves stale object references behind, so
  look objects up by name afterwards.
- `compile.py` mirrors the folder into the Workshop Tools content folder, wipes and recompiles
  the model, and mirrors the result into each install target.
- Installing into a running game fails, so stop the server and client first. The `.blend` stays
  out of the compile and goes into Git LFS.

## 9. Wire it into the plugin

- Point the plugin at the `.vmdl`, and update the scale, placement box, trigger radii and measured
  offsets to the new size. In Stronghold, `Scale` in `ItemAssets` resizes the model, its
  collision and its animation together, with no recompile.
- Start a loop from spawn with the `SetAnimationLooping` input; in Stronghold, name it in `Idle`
  in `ItemAssets`. `prop_dynamic` has no `SetAnimation` input (`game/core/base.fgd`). Play a
  one-shot with `SetAnimationNotLooping`, after `SetIdleAnimationLooping` names the loop to
  return to. Stronghold's `PlayAnimation` in `World/Effects` does both.
- Record new or replaced assets in the plugin's assets doc, with how they were made.
- Build with `build-local`, install with `uv run poe install <plugin>`, and check that the
  installed DLL's hash matches the build before the user tests it.

## 10. Clean up

The task is done when nothing it created is left without a purpose:

- **Blender:** `session.remove_scene` for each scene the task created that isn't the model, such
  as imported references, then `session.purge()`. Leave the user's own scenes alone.
- **Model folder:** `compile.py --prune` deletes source files that no `.vmdl`, `.vmat` or DMX
  names, such as replaced textures. Delete any stray export by hand: `anims/`, or a DMX named
  after a Blender object instead of the model.
- **Game folders:** `compile.py` already drops stale files from the model's own folders. If a
  model folder was renamed or removed, delete its old folder under `content/csgo_addons/<addon>/`,
  `game/csgo_addons/<addon>/` and `game/csgo/` on the client and the server.
- **Plugin:** remove constants, config and doc lines that only served the replaced model.
- **Scratch:** delete the scratchpad renders, drafts and scripts. Also delete the
  `~/.codex/generated_images/` folders the Codex runs added, comparing with the step 4 listing.
- **Git:** `git status` in the plugin repo shows only the files you meant to change. Commit with
  the `commit` skill when asked.
