#!/usr/bin/env python3
"""Update every submodule (cs2-kit and its nested SDKs) to the latest upstream commit.

Walks the submodule tree top-down: cs2-kit is advanced to the tip of its tracked
branch first, then its own submodules are re-initialised (so newly added ones are
picked up) and advanced in turn.

Each submodule is left on a real local branch, not a detached HEAD, so follow-up
work inside vendor/cs2-kit can be committed normally.

    uv run poe update-submodules            # everything to upstream tips
    uv run poe update-submodules --pinned   # nested ones to the commit cs2-kit pins
    uv run poe update-submodules --dry-run  # report what would move

Run via poe: it sets PYTHONPATH to the vendored kit's scripts/ for `import buildtools`.
"""

import argparse
import subprocess
from dataclasses import dataclass
from pathlib import Path

import buildtools

ROOT = Path(__file__).resolve().parents[1]


@dataclass(frozen=True)
class Submodule:
    """One entry from a .gitmodules file."""

    name: str
    path: Path
    """Absolute path to the submodule working tree."""
    display: str
    """Path relative to the repo root, for logging."""
    branch: str | None
    """Branch pinned in .gitmodules, or None to follow the remote's default."""


def git(repo: Path, *args: str) -> str:
    """Run git in `repo` and return its stdout, dying on a non-zero exit."""
    result = subprocess.run(
        ["git", "-C", str(repo), *args],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        buildtools.die(f"git {' '.join(args)} (in {repo}):\n{result.stderr.strip()}")
    return result.stdout.strip()


def read_submodules(repo: Path) -> list[Submodule]:
    """Parse `repo`'s .gitmodules into submodule entries."""
    if not (repo / ".gitmodules").is_file():
        return []

    config = git(repo, "config", "-f", ".gitmodules", "--list")
    paths: dict[str, str] = {}
    branches: dict[str, str] = {}
    for line in config.splitlines():
        key, _, value = line.partition("=")
        if not key.startswith("submodule."):
            continue
        name, _, field = key[len("submodule.") :].rpartition(".")
        if field == "path":
            paths[name] = value
        elif field == "branch":
            branches[name] = value

    return [
        Submodule(
            name=name,
            path=repo / path,
            display=str((repo / path).relative_to(ROOT)).replace("\\", "/"),
            branch=branches.get(name),
        )
        for name, path in sorted(paths.items(), key=lambda item: item[1])
    ]


def default_branch(repo: Path) -> str:
    """Resolve the branch origin's HEAD points at, e.g. 'main' or 'master'."""
    for line in git(repo, "ls-remote", "--symref", "origin", "HEAD").splitlines():
        if line.startswith("ref:"):
            return line.split()[1].removeprefix("refs/heads/")
    buildtools.die(f"cannot resolve the default branch of origin in {repo}")


def is_dirty(repo: Path) -> bool:
    """Whether `repo` has uncommitted changes to tracked files, ignoring gitlinks.

    Gitlinks are excluded because this script is what moves them: a submodule whose
    only change is a bumped child pointer is still safe to fast-forward.
    """
    status = git(repo, "status", "--porcelain", "--ignore-submodules=all")
    return bool(status)


def short(repo: Path, rev: str = "HEAD") -> str:
    """Abbreviated commit id of `rev`."""
    return git(repo, "rev-parse", "--short", rev)


def fetch_tip(sub: Submodule, branch: str) -> str:
    """Fetch `branch` from origin and return the fetched commit id."""
    shallow = git(sub.path, "rev-parse", "--is-shallow-repository") == "true"
    depth = ["--depth", "1"] if shallow else []
    git(sub.path, "fetch", *depth, "origin", branch)
    return git(sub.path, "rev-parse", "FETCH_HEAD")


def advance(sub: Submodule, *, dry_run: bool) -> bool:
    """Move `sub` onto the tip of its tracked branch. Returns True if it moved."""
    branch = sub.branch or default_branch(sub.path)
    before = git(sub.path, "rev-parse", "HEAD")
    target = fetch_tip(sub, branch)

    if before == target:
        print(f"  {sub.display}: up to date ({branch} @ {short(sub.path)})")
        return False

    if dry_run:
        print(f"  {sub.display}: {short(sub.path, before)} -> {short(sub.path, target)} ({branch})")
        return True

    if is_dirty(sub.path):
        print(f"  {sub.display}: SKIPPED - uncommitted changes, commit or stash them first")
        return False

    git(sub.path, "checkout", "-B", branch, target)
    git(sub.path, "branch", "--set-upstream-to", f"origin/{branch}", branch)
    print(f"  {sub.display}: {short(sub.path, before)} -> {short(sub.path, target)} ({branch})")
    return True


def restore_pinned(sub: Submodule, parent: Path) -> None:
    """Check `sub` out at the commit `parent` records for it."""
    git(parent, "submodule", "update", "--init", "--recursive", "--", str(sub.path))
    print(f"  {sub.display}: pinned @ {short(sub.path)}")


def walk(repo: Path, *, remote: bool, dry_run: bool, depth: int = 0) -> list[str]:
    """Update every submodule under `repo`, depth-first, parents before children."""
    moved: list[str] = []
    for sub in read_submodules(repo):
        if not (sub.path / ".git").exists():
            if dry_run:
                print(f"  {sub.display}: not initialised, would clone")
                continue
            git(repo, "submodule", "update", "--init", "--", str(sub.path))

        if remote or depth == 0:
            if advance(sub, dry_run=dry_run):
                moved.append(sub.display)
        else:
            restore_pinned(sub, repo)

        if not dry_run:
            # Re-init after the bump: the new commit may add or drop submodules.
            git(sub.path, "submodule", "sync", "--recursive")
            git(sub.path, "submodule", "update", "--init", "--recursive")

        moved += walk(sub.path, remote=remote, dry_run=dry_run, depth=depth + 1)

    return moved


def main() -> None:
    """Sync submodule URLs, then advance the tree and report what moved."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--pinned",
        action="store_true",
        help="update cs2-kit only; check its nested SDKs out at the commits it pins",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="fetch and report what would move, without checking anything out",
    )
    args = parser.parse_args()

    git(ROOT, "rev-parse", "--git-dir")  # die early when ROOT is not a git repo

    print("==> Syncing submodule URLs from .gitmodules")
    if not args.dry_run:
        git(ROOT, "submodule", "sync", "--recursive")

    print("==> Updating submodules" + (" (dry run)" if args.dry_run else ""))
    moved = walk(ROOT, remote=not args.pinned, dry_run=args.dry_run)

    if not moved:
        print("\nEverything is already at the latest commit.")
        return

    verb = "would move" if args.dry_run else "moved"
    print(f"\n{len(moved)} submodule(s) {verb}: {', '.join(moved)}")
    if args.dry_run:
        return

    print("\nRebuild before committing:  uv run poe build")
    if any(name != "vendor/cs2-kit" for name in moved):
        print(
            "Nested SDKs moved, so vendor/cs2-kit now has its own gitlink changes.\n"
            "Commit those inside the cs2-kit repo first, push, then re-point this repo at it."
        )


if __name__ == "__main__":
    main()
