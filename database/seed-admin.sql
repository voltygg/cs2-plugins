-- Seed an admin entry. Run AFTER schema.sql.
-- Edit the steam_id, name, and group(s) below for your server, then:
--   psql -d admin_system -f database/seed-admin.sql
-- Pick up the new admin without restarting the server with `!admin_reload` in chat.
INSERT INTO
  admins (steam_id, name, groups, flags, immunity)
VALUES
  (
    76561198153558892,
    -- <-- replace with your SteamID64
    '.NET Player',
    -- <-- replace with display name
    ARRAY [ 'superadmin' ] :: TEXT [ ],
    -- group memberships
    '',
    -- per-admin extra flags (usually empty; group provides flags)
    100 -- per-admin extra immunity (group's immunity is also considered)
  ) ON CONFLICT (steam_id) DO NOTHING;
