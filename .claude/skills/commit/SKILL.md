# Commit (cs2-plugins + voltmod)

Commits and pushes changes spanning this repo and a local voltmod checkout. The two
are separate repos joined by a Conan dependency, not a submodule, so there is no
pointer to bump. voltmod still lands **first**, because cs2-plugins resolves it
by git ref (`pyproject.toml`) and by package version (`conanfile.py`).

## When to use

The user says "commit", "commit and push", "push the changes", or similar after
editing `plugins/` and/or a voltmod checkout. If only one repo has changes, skip the
other's steps.

The kit checkout is wherever `conan editable list` points, commonly `vendor/voltmod`
(git-ignored here) or a sibling `../voltmod`. If neither exists, only this repo is in
play.

## Step-by-step

### 1. Gather state from both repos in parallel

```bash
git status
git diff HEAD --stat
git log --oneline -5
git -C <kit> status
git -C <kit> diff HEAD --stat
git -C <kit> log --oneline -3
```

- If both are clean, there is nothing to do. Say so and stop.
- If only cs2-plugins is dirty, skip to step 3.
- If voltmod is dirty, start at step 2.

### 2. Commit and push voltmod first

Read enough of `git -C <kit> diff HEAD` to write an accurate message, then:

```bash
git -C <kit> add <specific files>
git -C <kit> commit -m "$(cat <<'EOF'
<type>: <summary>

<optional body: why, not how>
EOF
)"
git -C <kit> push origin main
```

Stage by file name (no `git add -A`). Never stage secrets.

If the push is rejected because the remote moved, `git -C <kit> pull --rebase origin
main` and retry. Do not force-push.

**If the change alters the kit's public surface** (headers, `voltmod_add_plugin`, CMake
helpers), cs2-plugins cannot see it until a package exists. Tell the user, and let
them decide whether to:

- tag a release (`git -C <kit> tag vX.Y.Z && git push origin vX.Y.Z`), which runs
  `publish-conan.yml` and uploads the package; or
- keep working locally against `conan editable add <kit>`, in which case cs2-plugins
  must not be pushed with a `conan.lock` that names an unpublished recipe revision.

Do not tag on your own. A tag is a release.

### 3. Stage and commit cs2-plugins

```bash
git status
git diff --cached
git diff
git add <specific files>
git commit -m "$(cat <<'EOF'
<type>: <summary>

<optional body>
EOF
)"
```

Watch for two files that must move together with a kit release, never ahead of it:

- `conan.lock` pins `voltmod/<version>#<recipe-revision>`. A revision that is not
  on the remote turns CI red with `ERROR: Package not resolved`.
- `pyproject.toml` and `uv.lock` hold the `voltmod` git dependency. After pushing
  the kit, refresh with `uv lock --upgrade-package voltmod`.

### 4. Push cs2-plugins

```bash
git push origin main
```

Same rebase-on-conflict rule. Never force-push.

### 5. Report

End with this repo's new HEAD SHA and, if applicable, the voltmod HEAD SHA, so the
user can verify both landed.

## Commit message style

Match the existing history (`git log --oneline -10` to confirm). Prefixes in use:

- `feat:` new functionality
- `fix:` bug fix
- `refactor:` restructuring without behavior change
- `chore:` dependency bumps, config tweaks, version bumps
- `docs:` documentation only
- `ci:` workflow and pipeline changes

Append `!` (e.g. `refactor!:`) when consumers must change something.

**Rules:**

- Summary: imperative mood, lowercase, no trailing period, under 72 chars
- Body wrapped at 72, blank line after the summary
- Focus on **why**; the diff already shows what
- Do NOT append `Co-Authored-By` or any other trailer unless explicitly asked

## Important

- **Order matters:** voltmod before cs2-plugins. The reverse ships a lockfile or a
  version range pointing at something that does not exist yet.
- **Don't amend** previous commits unless asked.
- **Don't force-push** main in either repo.
- **Don't bypass hooks** (`--no-verify`, `--no-gpg-sign`). If a hook fails, fix the
  cause, re-stage, and make a NEW commit.
- If a repo has nothing to commit, skip it and continue.
