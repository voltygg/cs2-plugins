---
name: commit
description: Commit and push changes across cs2-plugins, the vendor/voltmod checkout and the closed-source plugin submodules, inner repos first. Use for "commit", "commit and push", "push the changes".
---

# Commit (cs2-plugins + voltmod + closed plugins)

Three kinds of repo. voltmod is joined by a Conan dependency, not a gitlink, and lands
first because cs2-plugins pins it by git ref (`pyproject.toml`) and package revision
(`conan.lock`); its checkout is wherever `conan editable list` points, usually
`vendor/voltmod`. The closed plugins under `plugins/` are real submodules: cs2-plugins
records the commit, so they must be pushed before the pointer is.

## Steps

1. **Inspect every repo** in parallel: `git status`, `git diff HEAD --stat`,
   `git log --oneline -5`, the same with `git -C <framework>`, and
   `git submodule foreach 'git status --short'`. All clean: say so and stop.
   Skip any repo with no changes.
2. **voltmod first.** Stage by file name, commit, `git -C <framework> push origin main`.
   On rejection, `pull --rebase` and retry. If the change alters the public
   surface (headers, CMake helpers), tell the user cs2-plugins cannot see it
   until a package is published; do not tag yourself.
3. **Closed plugins next.** In each changed `plugins/<name>`, stage by file name, commit,
   push. Never leave the gitlink pointing at an unpushed commit: CI and deploy clone
   the submodule from its remote.
4. **cs2-plugins last.** Stage by file name — including each moved `plugins/<name>`
   gitlink — commit, `git push origin main`. `conan.lock` must not name a recipe
   revision that is not on the remote. After a framework push, refresh with
   `uv lock --upgrade-package voltmod`.
5. **Report** the new HEAD SHA of each repo that changed.

## Message

One sentence, nothing else:

```
<type>[!]: <summary>
```

Imperative, lowercase, no period, under 72 chars. No body, no trailers,
no `Co-Authored-By`. A `!` change may add one body line naming what consumers
must change. Types: feat, fix, refactor, chore, docs, ci, style, test.

## Never

Amend, force-push, tag, `--no-verify`, `git add -A`, stage secrets.
Commit a closed plugin's source into cs2-plugins.
