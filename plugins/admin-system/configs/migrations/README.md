# Migrations

One folder per driver: `postgres/`, `mariadb/`, `sqlite/`. The migrator picks the folder
matching `database.driver` and applies `NNNN_name.sql` files in version order.

One statement per `;`; no procedure bodies. Keep the same version number meaning the same
change across dialects, but a dialect may skip a number when it needs no change there.

Never edit a file after it has been applied anywhere; add a new numbered file instead.
