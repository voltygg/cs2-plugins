---
name: commit
description: Commit and push changes across cs2-plugins and the vendor/voltmod checkout, voltmod first. Use for "commit", "commit and push", "push the changes".
---

# Commit (cs2-plugins + voltmod)

Two separate repos joined by a Conan dependency, not a submodule. voltmod lands
first because cs2-plugins pins it by git ref (`pyproject.toml`) and package
revision (`conan.lock`). The framework checkout is wherever `conan editable list`
points, usually `vendor/voltmod`.

## Steps

1. **Inspect both repos** in parallel: `git status`, `git diff HEAD --stat`,
   `git log --oneline -5`, and the same with `git -C <framework>`. Both clean: say so
   and stop. Skip any repo with no changes.
2. **voltmod first.** Stage by file name, commit, `git -C <framework> push origin main`.
   On rejection, `pull --rebase` and retry. If the change alters the public
   surface (headers, CMake helpers), tell the user cs2-plugins cannot see it
   until a package is published; do not tag yourself.
3. **cs2-plugins.** Stage by file name, commit, `git push origin main`.
   `conan.lock` must not name a recipe revision that is not on the remote.
   After a framework push, refresh with `uv lock --upgrade-package voltmod`.
4. **Report** the new HEAD SHA of each repo that changed.

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
