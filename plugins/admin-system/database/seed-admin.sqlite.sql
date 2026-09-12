-- Run after the plugin applies migrations. Edit the SteamID, name, and groups, then:
--   sqlite3 admin_system.db < plugins/admin-system/database/seed-admin.sqlite.sql
-- Run `!admin_reload` to apply the change without restarting.
INSERT OR IGNORE INTO
  admins (steam_id, name, groups, flags, immunity)
VALUES
  (
    76561198153558892,
    -- <-- replace with your SteamID64
    '.NET Player',
    -- <-- replace with display name
    '["super_admin"]',
    -- group memberships, JSON array text
    '',
    -- per-admin extra flags (usually empty; group provides flags)
    100 -- per-admin extra immunity (group's immunity is also considered)
  );

-- Optional server-specific group. `admins.groups` above remains global.
-- INSERT OR IGNORE INTO
--   admin_server_groups (admin_steam_id, server_tag, group_name)
-- VALUES
--   (76561198153558892, 'server-1', 'super_admin');
