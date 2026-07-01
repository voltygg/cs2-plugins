---
name: commit
description: "Commit and push changes across admin-system and the cs2-kit submodule, in the right order so the submodule pointer is never broken."
---

# Commit (admin-system + cs2-kit)

Commits and pushes changes spanning the admin-system repo and its `vendor/cs2-kit` submodule. Submodule changes are committed and pushed **first** so the parent's submodule pointer always references a commit that exists on the remote.

## When to use

The user says "commit", "commit and push", "push the changes", or similar after editing files in `src/` and/or `vendor/cs2-kit/`. If only one of the two repos has changes, the irrelevant steps are skipped.

## Step-by-step process

### 1. Gather state from both repos in parallel

```bash
git status
git diff HEAD --stat
git -C vendor/cs2-kit status
git -C vendor/cs2-kit diff HEAD --stat
git log --oneline -5
git -C vendor/cs2-kit log --oneline -3
```

Decide which path applies:

- **A. Both clean** - nothing to do. Tell the user and stop.
- **B. Only admin-system dirty** - skip to step 4.
- **C. cs2-kit dirty (with or without admin-system changes)** - start at step 2.

### 2. Commit and push cs2-kit first

Read enough of `git -C vendor/cs2-kit diff HEAD` to write an accurate message. Then:

```bash
git -C vendor/cs2-kit add <specific files>
git -C vendor/cs2-kit commit -m "$(cat <<'EOF'
<type>: <summary>

<optional body - why, not how>
EOF
)"
git -C vendor/cs2-kit push origin main
```

Stage by file name (no `git add -A`). Never stage secrets.

If the push fails because the remote has new commits, run `git -C vendor/cs2-kit pull --rebase origin main` and retry the push. Do not force-push.

### 3. Stage the bumped submodule pointer in admin-system

After the submodule push lands, the parent repo sees `vendor/cs2-kit` as modified (new SHA). Stage it:

```bash
git add vendor/cs2-kit
```

### 4. Stage and commit admin-system

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

If the commit only bumps the submodule (no other src changes), use `chore: bump cs2-kit to <reason>` - match the style of `bacf503` in recent history.

### 5. Push admin-system

```bash
git push origin main
```

Same rebase-on-conflict rule as step 2. Never force-push.

### 6. Report

End with the parent repo's new HEAD SHA and, if applicable, the cs2-kit HEAD SHA, so the user can verify both pushes landed.

## Commit message style

Match the existing history (`git log --oneline -10` to confirm). Conventional-commit prefixes used in this repo:

- `feat:` - new functionality
- `fix:` - bug fix
- `refactor:` - restructuring without behavior change
- `chore:` - submodule bumps, config tweaks, version bumps
- `docs:` - documentation only

**Rules:**

- Summary: imperative mood, lowercase, no trailing period, under 72 chars
- Body wrapped at 72; separate from summary with a blank line
- Focus on **why**, not **what** - the diff shows what
- Do NOT append `Co-Authored-By` or any other trailer unless the user explicitly asks

## Important

- **Order matters:** push cs2-kit before admin-system. The opposite order ships a parent commit pointing at an unpushed submodule SHA, which breaks fresh clones.
- **Don't amend** previous commits unless the user explicitly asks.
- **Don't force-push** main in either repo.
- **Don't bypass hooks** (`--no-verify`, `--no-gpg-sign`). Investigate and fix the underlying issue if a hook fails.
- If pre-commit hooks fail, the commit did not happen - fix, re-stage, create a NEW commit (not `--amend`).
- If there are no changes to commit in a given repo, skip that repo and continue.

## Examples

**Both repos changed (e.g., a feature that touched cs2-kit + admin-system consumers):**

```
# In vendor/cs2-kit:
feat: add Players module with Player and PlayerManager

Move generic player identity and slot/steamid lookup into cs2-kit
so any plugin can reuse them. Plugin-specific state stays in the
consumer's own managers.

# In admin-system:
refactor: use cs2-kit Players module

Drop local Players/ in favor of CS2Kit::Players::Player and
PlayerManager. Removes dead _isAdmin/_isMuted/_isGagged state
that was written but never read.
```

**Submodule bump only:**

```
chore: bump cs2-kit to pull in Players module
```
