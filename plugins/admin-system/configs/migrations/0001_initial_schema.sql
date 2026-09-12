-- admin-system schema. RunMigrations resolves the placeholders for the configured driver.
-- Add a change as a new numbered file; never edit one that has been applied.
-- Tables come first: `voltmod database tables` reads this file to generate the C++ specs.

CREATE TABLE IF NOT EXISTS admin_groups (
  id @ID@,
  name VARCHAR(64) UNIQUE NOT NULL,
  flags VARCHAR(64) NOT NULL DEFAULT '',
  immunity INTEGER NOT NULL DEFAULT 0,
  inherits TEXT NOT NULL DEFAULT '[]',
  chat_prefix VARCHAR(64) NOT NULL DEFAULT '',
  prefix_color VARCHAR(32) NOT NULL DEFAULT '',
  name_color VARCHAR(32) NOT NULL DEFAULT '',
  message_color VARCHAR(32) NOT NULL DEFAULT '',
  created_at BIGINT NOT NULL DEFAULT @NOW@,
  updated_at BIGINT NOT NULL DEFAULT @NOW@
);

CREATE TABLE IF NOT EXISTS admins (
  id @ID@,
  steam_id BIGINT UNIQUE NOT NULL,
  name VARCHAR(128) NOT NULL,
  groups TEXT NOT NULL DEFAULT '[]',
  flags VARCHAR(64) NOT NULL DEFAULT '',
  immunity INTEGER NOT NULL DEFAULT 0,
  -- Empty colors inherit from the group; display_prefix hides only the prefix.
  display_prefix BOOLEAN NOT NULL DEFAULT @TRUE@,
  name_color VARCHAR(32) NOT NULL DEFAULT '',
  message_color VARCHAR(32) NOT NULL DEFAULT '',
  -- Per-admin language for the in-game admin panel. Defaults to English regardless of plugin.locale.
  language VARCHAR(5) NOT NULL DEFAULT 'en',
  created_at BIGINT NOT NULL DEFAULT @NOW@,
  updated_at BIGINT NOT NULL DEFAULT @NOW@,
  is_frozen BOOLEAN NOT NULL DEFAULT @FALSE@,
  frozen_at BIGINT NOT NULL DEFAULT 0,
  frozen_by BIGINT NOT NULL DEFAULT 0,
  freeze_reason TEXT NOT NULL DEFAULT ''
);

CREATE TABLE IF NOT EXISTS players (
  id @ID@,
  steam_id BIGINT UNIQUE NOT NULL,
  name VARCHAR(128) NOT NULL,
  ip_address VARCHAR(45) NOT NULL DEFAULT '',
  first_seen BIGINT NOT NULL DEFAULT @NOW@,
  last_seen BIGINT NOT NULL DEFAULT @NOW@,
  total_connections INTEGER NOT NULL DEFAULT 1,
  total_playtime BIGINT NOT NULL DEFAULT 0
);

-- Every punishment an admin can issue. They differ only in what the server does with them, so
-- `kind` is the whole difference: target_ip is filled for a ban, a warning carries no duration.
CREATE TABLE IF NOT EXISTS punishments (
  id @ID@,
  kind VARCHAR(16) NOT NULL,   -- ban | voice_mute | text_mute | warn
  target_steam_id BIGINT NOT NULL,
  target_name VARCHAR(128) NOT NULL,
  target_ip VARCHAR(45) NOT NULL DEFAULT '',
  admin_steam_id BIGINT NOT NULL DEFAULT 0,
  admin_name VARCHAR(128) NOT NULL,
  reason TEXT NOT NULL,
  created_at BIGINT NOT NULL DEFAULT @NOW@,
  -- 0 means permanent. `duration` is what the admin typed, `expires_at` is when it lapses.
  expires_at BIGINT NOT NULL DEFAULT 0,
  duration BIGINT NOT NULL DEFAULT 0,
  is_active BOOLEAN NOT NULL DEFAULT @TRUE@,
  removed_at BIGINT NOT NULL DEFAULT 0,
  removed_by BIGINT NOT NULL DEFAULT 0,
  removed_reason TEXT NOT NULL DEFAULT ''
);

-- Servers upsert by `server.tag` and update last_seen every minute.
CREATE TABLE IF NOT EXISTS servers (
  id @ID@,
  tag VARCHAR(64) UNIQUE NOT NULL,
  name VARCHAR(128) NOT NULL DEFAULT '',
  created_at BIGINT NOT NULL DEFAULT @NOW@,
  last_seen BIGINT NOT NULL DEFAULT @NOW@
);

-- Adds server-specific groups; admins.groups remains global.
CREATE TABLE IF NOT EXISTS admin_server_groups (
  id @ID@,
  admin_steam_id BIGINT NOT NULL,
  server_tag VARCHAR(64) NOT NULL,
  group_name VARCHAR(64) NOT NULL,
  created_at BIGINT NOT NULL DEFAULT @NOW@,
  UNIQUE (admin_steam_id, server_tag, group_name)
);

-- Admin action audit trail + the abuse-rate detection data source (kicks live only here).
-- action: kick | ban | voice_mute | text_mute | warn | freeze_admin | unfreeze_admin
CREATE TABLE IF NOT EXISTS admin_activity (
  id @ID@,
  admin_steam_id BIGINT NOT NULL,
  admin_name VARCHAR(128) NOT NULL DEFAULT '',
  action VARCHAR(32) NOT NULL,
  target_steam_id BIGINT NOT NULL DEFAULT 0,
  target_name VARCHAR(128) NOT NULL DEFAULT '',
  detail TEXT NOT NULL DEFAULT '',
  server_tag VARCHAR(64) NOT NULL DEFAULT '',
  created_at BIGINT NOT NULL DEFAULT @NOW@
);

-- The game server inserts through created_at. The website owns later triage columns.
CREATE TABLE IF NOT EXISTS player_reports (
  id @ID@,
  reporter_steam_id BIGINT NOT NULL,
  reporter_name VARCHAR(128) NOT NULL DEFAULT '',
  reporter_ip VARCHAR(45) NOT NULL DEFAULT '',
  target_steam_id BIGINT NOT NULL,
  target_name VARCHAR(128) NOT NULL DEFAULT '',
  target_ip VARCHAR(45) NOT NULL DEFAULT '',
  -- Stable grouping key from settings.jsonc `reports.reasons[].code`; 'other' for free text.
  reason_code VARCHAR(32) NOT NULL DEFAULT '',
  -- The preset label, or the text the reporter typed.
  reason TEXT NOT NULL DEFAULT '',
  server_tag VARCHAR(64) NOT NULL DEFAULT '',
  map_name VARCHAR(64) NOT NULL DEFAULT '',
  created_at BIGINT NOT NULL DEFAULT @NOW@,
  -- Website-owned triage columns; the plugin never writes these.
  status VARCHAR(16) NOT NULL DEFAULT 'open',  -- open | reviewing | resolved | rejected | duplicate
  handled_by BIGINT NOT NULL DEFAULT 0,        -- admin steam id that closed it
  handled_at BIGINT NOT NULL DEFAULT 0,
  resolution TEXT NOT NULL DEFAULT ''
);

CREATE INDEX IF NOT EXISTS idx_admins_steam_id ON admins(steam_id);

CREATE INDEX IF NOT EXISTS idx_players_steam_id ON players(steam_id);
CREATE INDEX IF NOT EXISTS idx_players_last_seen ON players(last_seen DESC);

-- "is this player punished right now", then the load-time read and the expiry sweep.
CREATE INDEX IF NOT EXISTS idx_punishments_target ON punishments(target_steam_id, kind, is_active, expires_at);
CREATE INDEX IF NOT EXISTS idx_punishments_active ON punishments(kind, is_active, expires_at);
CREATE INDEX IF NOT EXISTS idx_punishments_target_ip ON punishments(target_ip);

CREATE INDEX IF NOT EXISTS idx_admin_server_groups_lookup ON admin_server_groups(server_tag, admin_steam_id);
CREATE INDEX IF NOT EXISTS idx_admin_activity_admin_time ON admin_activity(admin_steam_id, created_at DESC);

-- Website queue: newest open reports first.
CREATE INDEX IF NOT EXISTS idx_player_reports_status_time ON player_reports(status, created_at DESC);
-- "every report against this player" - the main triage drill-down.
CREATE INDEX IF NOT EXISTS idx_player_reports_target_time ON player_reports(target_steam_id, created_at DESC);
-- Reporter history / false-report abuse detection.
CREATE INDEX IF NOT EXISTS idx_player_reports_reporter_time ON player_reports(reporter_steam_id, created_at DESC);
-- Per-server dashboards and filtering.
CREATE INDEX IF NOT EXISTS idx_player_reports_server_time ON player_reports(server_tag, created_at DESC);

@INSERT_IF_ABSENT@ admin_groups (name, flags, immunity, inherits, chat_prefix, prefix_color, name_color, message_color)
VALUES ('super_admin', 'z', 100, '[]', '[ROOT]', 'red', 'lightblue', 'default')
@ON_CONFLICT(name)@;
