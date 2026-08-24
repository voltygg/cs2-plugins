-- 0002 Per-server admin scoping + admin-abuse freeze protection
-- Applied by VoltMod::RunMigrations. Add a new numbered file for later changes.

-- Servers upsert by `server.tag` and update last_seen every minute.
CREATE TABLE IF NOT EXISTS servers (
  id BIGSERIAL PRIMARY KEY,
  tag VARCHAR(64) UNIQUE NOT NULL,
  name VARCHAR(128) NOT NULL DEFAULT '',
  created_at BIGINT NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW())::BIGINT,
  last_seen BIGINT NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW())::BIGINT
);

-- Adds server-specific groups; admins.groups remains global.
CREATE TABLE IF NOT EXISTS admin_server_groups (
  id BIGSERIAL PRIMARY KEY,
  admin_steam_id BIGINT NOT NULL,
  server_tag VARCHAR(64) NOT NULL,
  group_name VARCHAR(64) NOT NULL,
  created_at BIGINT NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW())::BIGINT,
  UNIQUE (admin_steam_id, server_tag, group_name)
);

-- Admin action audit trail + the abuse-rate detection data source (kicks live only here).
-- action: kick | ban | voice_mute | text_mute | warn | freeze_admin | unfreeze_admin
CREATE TABLE IF NOT EXISTS admin_activity (
  id BIGSERIAL PRIMARY KEY,
  admin_steam_id BIGINT NOT NULL,
  admin_name VARCHAR(128) NOT NULL DEFAULT '',
  action VARCHAR(32) NOT NULL,
  target_steam_id BIGINT NOT NULL DEFAULT 0,
  target_name VARCHAR(128) NOT NULL DEFAULT '',
  detail TEXT NOT NULL DEFAULT '',
  server_tag VARCHAR(64) NOT NULL DEFAULT '',
  created_at BIGINT NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW())::BIGINT
);

-- Freeze state is network-wide. frozen_by = 0 marks an automatic freeze;
-- admin_activity records freeze history.
ALTER TABLE admins ADD COLUMN IF NOT EXISTS is_frozen BOOLEAN NOT NULL DEFAULT FALSE;
ALTER TABLE admins ADD COLUMN IF NOT EXISTS frozen_at BIGINT NOT NULL DEFAULT 0;
ALTER TABLE admins ADD COLUMN IF NOT EXISTS frozen_by BIGINT NOT NULL DEFAULT 0;
ALTER TABLE admins ADD COLUMN IF NOT EXISTS freeze_reason TEXT NOT NULL DEFAULT '';

CREATE INDEX IF NOT EXISTS idx_admin_server_groups_lookup ON admin_server_groups(server_tag, admin_steam_id);
CREATE INDEX IF NOT EXISTS idx_admin_activity_admin_time ON admin_activity(admin_steam_id, created_at DESC);
