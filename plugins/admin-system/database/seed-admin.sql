-- Seed the first admin. Edit the SteamID, name and groups, then render it for your driver and
-- pipe it into a client:
--   uv run voltmod database sql plugins/admin-system/database/seed-admin.sql --driver postgres | psql -d admin_system
-- Run `!admin_reload` afterwards to pick it up without a restart.
@INSERT_IF_ABSENT@ admins (steam_id, name, groups, permissions)
VALUES (
  76561198153558892,   -- your SteamID64
  '.NET Player',       -- display name
  '["super_admin"]',   -- group memberships, JSON array text
  '[]'                 -- extra permissions on top of the group's, JSON array text
)
@ON_CONFLICT(steam_id)@;

-- A lower-level role and admin to test against: punishing needs strictly higher immunity.
@INSERT_IF_ABSENT@ admin_groups (name, permissions, immunity, inherits, chat_prefix, prefix_color, name_color, message_color)
VALUES ('admin', '["admin.freeze_admins","admin.kick","admin.ban","admin.unban","admin.mute","admin.control","admin.fun","admin.health","admin.hide","admin.wallhack","admin.bhop","admin.map","admin.weapon","admin.fun_mode","admin.vote"]', 50, '[]', '[ADMIN]', 'green', 'default', 'default')
@ON_CONFLICT(name)@;

-- The UPDATE demotes a SteamID that already holds a higher role.
@INSERT_IF_ABSENT@ admins (steam_id, name, groups, permissions)
VALUES (76561198093475210, 'Hikka', '["admin"]', '[]')
@ON_CONFLICT(steam_id)@;
UPDATE admins SET groups = '["admin"]', permissions = '[]' WHERE steam_id = 76561198093475210;
