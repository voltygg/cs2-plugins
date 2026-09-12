"""Render the admin-system schema per driver and generate its sqlpp23 table specs.

`schema.sql.in` is the one hand-written copy. Rendering resolves the few places the dialects
disagree; sqlpp23-ddl2cpp then reads the Postgres render, skipping what its grammar cannot
parse (the indexes and the seed row).
"""

from __future__ import annotations

import argparse
import difflib
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PLUGIN = ROOT / "plugins" / "admin-system"
SCHEMA_IN = PLUGIN / "schema" / "schema.sql.in"
SEED_IN = PLUGIN / "schema" / "seed-admin.sql.in"
MIGRATIONS = PLUGIN / "configs" / "migrations"
SEED_DIR = PLUGIN / "database"
RENDERED_NAME = "0001_initial_schema.sql"
HEADER = PLUGIN / "src" / "Database" / "Tables" / "Schema.hpp"
HEADER_TEMPLATE = PLUGIN / "schema" / "header.in"
NAMESPACE = "AdminSystem::Database::Tables"

BANNER = (
    "-- Generated from schema/schema.sql.in by `poe schema`. Do not edit.\n"
    "-- Applied by VoltMod::RunMigrations; add a new numbered file for later changes.\n\n"
)

DIALECTS = {
    "postgres": {
        "ID": "BIGSERIAL PRIMARY KEY",
        "NOW": "EXTRACT(EPOCH FROM NOW())::BIGINT",
        "TRUE": "TRUE",
        "FALSE": "FALSE",
        "INSERT_IF_ABSENT": "INSERT INTO",
    },
    "mariadb": {
        "ID": "BIGINT AUTO_INCREMENT PRIMARY KEY",
        "NOW": "(UNIX_TIMESTAMP())",
        "TRUE": "TRUE",
        "FALSE": "FALSE",
        "INSERT_IF_ABSENT": "INSERT IGNORE INTO",
    },
    "sqlite": {
        # 1/0 rather than TRUE/FALSE: it is what an existing database's stored schema text says.
        "ID": "INTEGER PRIMARY KEY AUTOINCREMENT",
        "NOW": "(strftime('%s','now'))",
        "TRUE": "1",
        "FALSE": "0",
        "INSERT_IF_ABSENT": "INSERT OR IGNORE INTO",
    },
}

_TOKEN = re.compile(r"@([A-Z_]+)@")
# Two tokens, because Postgres puts its clause at the end of the statement and the other two
# at the front.
_ON_CONFLICT = re.compile(r"@ON_CONFLICT\(([^)]*)\)@")


def render_one(template: str, driver: str) -> str:
    """Resolve every placeholder for `driver`; an unknown one is an error, never empty text."""
    values = DIALECTS[driver]

    def replace(match: re.Match[str]) -> str:
        name = match.group(1)
        if name not in values:
            raise SystemExit(f"schema.sql.in: unknown placeholder @{name}@")
        return values[name]

    conflict = "ON CONFLICT ({}) DO NOTHING" if driver == "postgres" else ""
    body = _ON_CONFLICT.sub(lambda m: conflict.format(m.group(1)), template)
    body = _TOKEN.sub(replace, body)
    # Without a conflict clause the ";" is left stranded on its own line.
    return re.sub(r"\)\n(-- )?;", ");", body)


def _strip_banner(text: str) -> str:
    """Drop the leading comment block; each rendered file gets a banner of its own."""
    lines = text.splitlines(True)
    cut = 0
    while cut < len(lines) and (lines[cut].startswith("--") or not lines[cut].strip()):
        cut += 1
    return "".join(lines[cut:])


def render() -> dict[Path, str]:
    """Every file the templates produce, keyed by the path it belongs at."""
    # The template banner documents the placeholders; rendered files carry their own.
    schema = _strip_banner(SCHEMA_IN.read_text(encoding="utf-8"))
    seed = SEED_IN.read_text(encoding="utf-8")

    out = {}
    for driver in DIALECTS:
        out[MIGRATIONS / driver / RENDERED_NAME] = BANNER + render_one(schema, driver)
        out[SEED_DIR / f"seed-admin.{driver}.sql"] = render_one(seed, driver)
    return out


def _write(files: dict[Path, str]) -> None:
    for path, text in files.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8", newline="\n")


def _ddl2cpp() -> Path:
    """conan's generated data file is the only handle on the script sqlpp23 ships."""
    for data in sorted((ROOT / "build").glob("*/generators/Sqlpp23-*-data.cmake")):
        match = re.search(
            r'set\(sqlpp23_PACKAGE_FOLDER_\w+ "([^"]+)"\)', data.read_text(encoding="utf-8")
        )
        if match:
            script = Path(match.group(1)) / "bin" / "sqlpp23-ddl2cpp"
            if script.is_file():
                return script
    raise SystemExit(
        "sqlpp23-ddl2cpp not found; run `poe build` once so conan resolves the package"
    )


def generate(ddl: str) -> str:
    """Run sqlpp23-ddl2cpp over the Postgres render; input and output stay out of the tree."""
    with tempfile.TemporaryDirectory() as work:
        source = Path(work) / RENDERED_NAME
        source.write_text(ddl, encoding="utf-8", newline="\n")
        target = Path(work) / HEADER.name
        argv = [
            sys.executable,
            str(_ddl2cpp()),
            "--path-to-ddl",
            str(source),
            "--path-to-header",
            str(target),
            "--namespace",
            NAMESPACE,
            "--naming-style",
            "camel-case",
            "--assume-auto-id",
            "--path-to-custom-template",
            str(HEADER_TEMPLATE),
        ]
        subprocess.run(argv, check=True, cwd=ROOT)
        return target.read_text(encoding="utf-8")


def check(files: dict[Path, str]) -> int:
    """Fail when a committed file no longer matches the templates. Writes nothing."""
    problems = [
        str(path.relative_to(ROOT))
        for path, text in files.items()
        if (path.read_text(encoding="utf-8") if path.is_file() else "") != text
    ]

    generated = generate(files[MIGRATIONS / "postgres" / RENDERED_NAME])
    current = HEADER.read_text(encoding="utf-8") if HEADER.is_file() else ""
    if current != generated:
        problems.append(str(HEADER.relative_to(ROOT)))
        sys.stdout.writelines(
            difflib.unified_diff(
                current.splitlines(True), generated.splitlines(True), "committed", "generated"
            )
        )

    if problems:
        print("Out of date, run `poe schema`: " + ", ".join(problems))
        return 1
    print("Schema files are up to date.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check", action="store_true", help="fail instead of writing when out of date"
    )
    args = parser.parse_args()

    files = render()
    if args.check:
        return check(files)

    _write(files)
    HEADER.write_text(
        generate(files[MIGRATIONS / "postgres" / RENDERED_NAME]), encoding="utf-8", newline="\n"
    )
    print(f"Rendered {len(files)} SQL files and {HEADER.relative_to(ROOT)}.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
