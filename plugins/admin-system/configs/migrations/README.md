# Migrations

Generated. The one hand-written copy is `../../schema/schema.sql.in`; `uv run poe schema`
renders it into `postgres/`, `mariadb/` and `sqlite/`, and regenerates the C++ table specs
in `src/Database/Tables/Schema.hpp` from the Postgres render. `poe lint` fails when a
rendered file no longer matches the template.

Placeholders cover the only places the dialects disagree: `@ID@` (auto-increment key),
`@NOW@` (current epoch), `@TRUE@` / `@FALSE@`, and `@INSERT_IF_ABSENT@` with
`@ON_CONFLICT(cols)@` for insert-if-absent, which Postgres spells at the end of the
statement and the others at the front.

The migrator applies `NNNN_name.sql` in version order, one statement per `;`, no procedure
bodies. Add a change as a new numbered file; never edit one that has been applied anywhere.
Table definitions stay ahead of anything referencing them, because the generator reads the
rendered file and needs each table before its first use.
