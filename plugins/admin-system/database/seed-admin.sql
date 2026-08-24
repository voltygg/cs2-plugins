-- Run after the plugin applies migrations. Edit the SteamID, name, and groups, then:
--   psql -d admin_system -f plugins/admin-system/database/seed-admin.sql
-- Run `!admin_reload` to apply the change without restarting.
INSERT INTO
  admins (steam_id, name, groups, flags, immunity)
VALUES
  (
    76561198153558892,
    -- <-- replace with your SteamID64
    '.NET Player',
    -- <-- replace with display name
    ARRAY [ 'super_admin' ] :: TEXT [ ],
    -- group memberships
    '',
    -- per-admin extra flags (usually empty; group provides flags)
    100 -- per-admin extra immunity (group's immunity is also considered)
  ) ON CONFLICT (steam_id) DO NOTHING;

-- Optional server-specific group. `admins.groups` above remains global.
-- INSERT INTO
--   admin_server_groups (admin_steam_id, server_tag, group_name)
-- VALUES
--   (76561198153558892, 'server-1', 'super_admin')
-- ON CONFLICT (admin_steam_id, server_tag, group_name) DO NOTHING;
