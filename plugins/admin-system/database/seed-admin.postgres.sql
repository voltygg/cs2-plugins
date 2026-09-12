-- Seed the first admin. Edit the SteamID, name and groups, then run it against your database:
--   psql -d admin_system -f plugins/admin-system/database/seed-admin.postgres.sql
--   mariadb admin_system < plugins/admin-system/database/seed-admin.mariadb.sql
--   sqlite3 <path> < plugins/admin-system/database/seed-admin.sqlite.sql
-- Run `!admin_reload` afterwards to pick it up without a restart.
INSERT INTO admins (steam_id, name, groups, flags, immunity)
VALUES (
  76561198153558892,   -- your SteamID64
  '.NET Player',       -- display name
  '["super_admin"]',   -- group memberships, JSON array text
  '',                  -- extra flags on top of the group's
  100                  -- extra immunity; the group's is considered too
)
ON CONFLICT (steam_id) DO NOTHING;

-- Optional server-specific group. `admins.groups` above stays global.
-- INSERT INTO admin_server_groups (admin_steam_id, server_tag, group_name)
-- VALUES (76561198153558892, 'server-1', 'super_admin')
-- ON CONFLICT (admin_steam_id, server_tag, group_name) DO NOTHING;
