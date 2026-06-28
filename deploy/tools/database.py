"""Shared PostgreSQL provisioning for deployed plugins."""

from __future__ import annotations

import os
import shlex

import inventory
from common import die, load_server_env, run
from remote import run_ssh


def ensure_databases(server_id: str | None, admin_user: str, *, dry_run: bool) -> None:
    """Create the shared app role and plugin databases when missing."""
    data = inventory.load()
    server = None
    if server_id:
        server = inventory.find_server(data, server_id)
        load_server_env(server_id, required=False)

    db_password = os.environ.get("DB_PASSWORD", "")
    pg_password = os.environ.get("PGPASSWORD", "")
    if not db_password:
        die("DB_PASSWORD must be set (the app role's password).")
    if not pg_password:
        die("PGPASSWORD must be set (the admin/superuser password).")
    if dry_run and server is None:
        die("--dry-run requires --server (it previews over-SSH commands).")

    conn = inventory.db_conn(data)
    app_user = str(conn["DB_USER"])
    port = os.environ.get("PGPORT", str(conn["DB_PORT"]))
    host = os.environ.get("PGHOST", "localhost" if server else str(conn["DB_HOST"]))

    def psql_admin(sql: str, *, capture: bool = True) -> str:
        if server:
            remote = " ".join(
                [
                    f"PGPASSWORD={shlex.quote(pg_password)}",
                    *(shlex.quote(part) for part in psql_command(host, port, admin_user, sql)),
                ]
            )
            completed = run_ssh(server, remote, dry_run=dry_run, capture=capture)
            return (completed.stdout or "").strip()

        env = os.environ.copy()
        env["PGPASSWORD"] = pg_password
        completed = run(psql_command(host, port, admin_user, sql), env=env, capture=capture)
        return (completed.stdout or "").strip()

    print(f"=== Ensuring shared PostgreSQL on {host}:{port} (admin: {admin_user}) ===")
    _ensure_role(psql_admin, app_user, db_password)
    for plugin in inventory.used_plugins(data):
        _ensure_database(psql_admin, plugin, inventory.plugin_db(data, plugin), app_user)
    print("=== Done ===")


def psql_command(host: str, port: str, admin_user: str, sql: str) -> list[str]:
    """Build a psql command for one admin SQL statement."""
    return [
        "psql",
        "-h",
        host,
        "-p",
        port,
        "-U",
        admin_user,
        "-d",
        "postgres",
        "-v",
        "ON_ERROR_STOP=1",
        "-qtA",
        "-c",
        sql,
    ]


def sql_identifier(value: str) -> str:
    """Quote a PostgreSQL identifier."""
    return '"' + value.replace('"', '""') + '"'


def sql_literal(value: str) -> str:
    """Quote a PostgreSQL string literal."""
    return "'" + value.replace("'", "''") + "'"


def _ensure_role(psql_admin, app_user: str, db_password: str) -> None:
    if psql_admin(f"SELECT 1 FROM pg_roles WHERE rolname={sql_literal(app_user)}") == "1":
        print(f"  role {app_user}: exists")
        return
    psql_admin(
        f"CREATE ROLE {sql_identifier(app_user)} LOGIN PASSWORD {sql_literal(db_password)}",
        capture=False,
    )
    print(f"  role {app_user}: created")


def _ensure_database(psql_admin, plugin: str, db: str, app_user: str) -> None:
    if psql_admin(f"SELECT 1 FROM pg_database WHERE datname={sql_literal(db)}") == "1":
        print(f"  db {db} ({plugin}): exists")
    else:
        psql_admin(
            f"CREATE DATABASE {sql_identifier(db)} OWNER {sql_identifier(app_user)}",
            capture=False,
        )
        print(f"  db {db} ({plugin}): created")
    psql_admin(
        f"GRANT ALL PRIVILEGES ON DATABASE {sql_identifier(db)} TO {sql_identifier(app_user)}",
        capture=False,
    )
