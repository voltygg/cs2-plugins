-- Seed an admin entry. Run AFTER schema.sql.
-- Edit the steam_id, name, and group(s) below for your server, then:
--   psql -d admin_system -f plugins/admin-system/database/seed-admin.sql
-- Pick up the new admin without restarting the server with `!admin_reload` in chat.
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

-- Optional: grant a group only on ONE server. `admins.groups` above is GLOBAL (every server
-- sharing the database); rows in admin_server_groups apply only where the server's
-- settings.jsonc `server.tag` matches. Pick up changes with `!admin_reload`.
-- INSERT INTO
--   admin_server_groups (admin_steam_id, server_tag, group_name)
-- VALUES
--   (76561198153558892, 'server-1', 'super_admin')
-- ON CONFLICT (admin_steam_id, server_tag, group_name) DO NOTHING;
