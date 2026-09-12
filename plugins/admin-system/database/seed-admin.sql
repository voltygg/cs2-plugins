-- Seed the first admin. Edit the SteamID, name and groups, then render it for your driver and
-- pipe it into a client:
--   uv run voltmod database sql plugins/admin-system/database/seed-admin.sql --driver postgres | psql -d admin_system
-- Run `!admin_reload` afterwards to pick it up without a restart.
@INSERT_IF_ABSENT@ admins (steam_id, name, groups, flags, immunity)
VALUES (
  76561198153558892,   -- your SteamID64
  '.NET Player',       -- display name
  '["super_admin"]',   -- group memberships, JSON array text
  '',                  -- extra flags on top of the group's
  100                  -- extra immunity; the group's is considered too
)
@ON_CONFLICT(steam_id)@;

-- Optional server-specific group. `admins.groups` above stays global.
-- @INSERT_IF_ABSENT@ admin_server_groups (admin_steam_id, server_tag, group_name)
-- VALUES (76561198153558892, 'server-1', 'super_admin')
-- @ON_CONFLICT(admin_steam_id, server_tag, group_name)@;
