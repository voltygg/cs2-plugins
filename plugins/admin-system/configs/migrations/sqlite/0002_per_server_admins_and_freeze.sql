-- 0002 Per-server admin scoping + admin-abuse freeze protection
-- Applied by VoltMod::RunMigrations. Add a new numbered file for later changes.
-- Freeze columns already live on admins from 0001; this file only adds the new tables.

-- Servers upsert by `server.tag` and update last_seen every minute.
CREATE TABLE IF NOT EXISTS servers (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  tag VARCHAR(64) UNIQUE NOT NULL,
  name VARCHAR(128) NOT NULL DEFAULT '',
  created_at BIGINT NOT NULL DEFAULT (strftime('%s','now')),
  last_seen BIGINT NOT NULL DEFAULT (strftime('%s','now'))
);

-- Adds server-specific groups; admins.groups remains global.
CREATE TABLE IF NOT EXISTS admin_server_groups (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  admin_steam_id BIGINT NOT NULL,
  server_tag VARCHAR(64) NOT NULL,
  group_name VARCHAR(64) NOT NULL,
  created_at BIGINT NOT NULL DEFAULT (strftime('%s','now')),
  UNIQUE (admin_steam_id, server_tag, group_name)
);

-- Admin action audit trail + the abuse-rate detection data source (kicks live only here).
-- action: kick | ban | voice_mute | text_mute | warn | freeze_admin | unfreeze_admin
CREATE TABLE IF NOT EXISTS admin_activity (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  admin_steam_id BIGINT NOT NULL,
  admin_name VARCHAR(128) NOT NULL DEFAULT '',
  action VARCHAR(32) NOT NULL,
  target_steam_id BIGINT NOT NULL DEFAULT 0,
  target_name VARCHAR(128) NOT NULL DEFAULT '',
  detail TEXT NOT NULL DEFAULT '',
  server_tag VARCHAR(64) NOT NULL DEFAULT '',
  created_at BIGINT NOT NULL DEFAULT (strftime('%s','now'))
);

CREATE INDEX IF NOT EXISTS idx_admin_server_groups_lookup ON admin_server_groups(server_tag, admin_steam_id);
CREATE INDEX IF NOT EXISTS idx_admin_activity_admin_time ON admin_activity(admin_steam_id, created_at DESC);
