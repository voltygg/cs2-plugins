---
name: commit
description: Commit and push changes across cs2-plugins, the voltmod checkout and the closed-source plugin submodules, inner repos first. Use for "commit", "commit and push", "push the changes".
---

# Commit

Push inner repos first: CI clones each submodule at the recorded commit, and cs2-plugins pins
voltmod by `conan.lock` and `uv.lock`. The `voltmod` gitlink itself is ignored (`ignore = all`)
and not a build input; leave it.

1. **Inspect** in one parallel batch: `git status --short`, `git diff HEAD --stat`,
   `git log --oneline -5` in the root and in `voltmod`, plus
   `git submodule foreach 'git status --short'`. Nothing changed anywhere: say so and stop.
2. **voltmod**, then each changed **closed plugin** (`plugins/anticheat`, `plugins/stronghold`):
   stage by name, commit, push `main`. On rejection `pull --rebase` and push again. A voltmod
   change to headers or CMake reaches cs2-plugins only through a release (`release` skill); say so.
3. **cs2-plugins**: stage by name, including each moved `plugins/<name>` gitlink; commit; push.
   `conan.lock` must not pin a voltmod revision the remote lacks. After a voltmod push, run
   `uv lock --upgrade-package voltmod` and include `uv.lock`.
4. **Report** the new HEAD of each repo that changed.

## Message

`<type>[!]: <summary>`: one line, imperative, lowercase, no period, under 72 characters. Types:
feat, fix, refactor, chore, docs, ci, style, test. No body, trailers or `Co-Authored-By`, except
that a `!` commit adds one body line saying what consumers must change.

## Never

Amend, force-push, tag, `--no-verify`, `git add -A`, stage secrets, or commit a closed plugin's
source into cs2-plugins.
