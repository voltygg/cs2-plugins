-- 0001 Initial schema
-- Applied by VoltMod::RunMigrations. Add a new numbered file for later changes.

CREATE TABLE IF NOT EXISTS admin_groups (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  name VARCHAR(64) UNIQUE NOT NULL,
  flags VARCHAR(64) NOT NULL DEFAULT '',
  immunity INTEGER NOT NULL DEFAULT 0,
  inherits TEXT NOT NULL DEFAULT '[]',
  chat_prefix VARCHAR(64) NOT NULL DEFAULT '',
  prefix_color VARCHAR(32) NOT NULL DEFAULT '',
  name_color VARCHAR(32) NOT NULL DEFAULT '',
  message_color VARCHAR(32) NOT NULL DEFAULT '',
  created_at BIGINT NOT NULL DEFAULT (strftime('%s','now')),
  updated_at BIGINT NOT NULL DEFAULT (strftime('%s','now'))
);

CREATE TABLE IF NOT EXISTS admins (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  steam_id BIGINT UNIQUE NOT NULL,
  name VARCHAR(128) NOT NULL,
  groups TEXT NOT NULL DEFAULT '[]',
  flags VARCHAR(64) NOT NULL DEFAULT '',
  immunity INTEGER NOT NULL DEFAULT 0,
  -- Empty colors inherit from the group; display_prefix hides only the prefix.
  display_prefix BOOLEAN NOT NULL DEFAULT 1,
  name_color VARCHAR(32) NOT NULL DEFAULT '',
  message_color VARCHAR(32) NOT NULL DEFAULT '',
  -- Per-admin language for the in-game admin panel. Defaults to English regardless of plugin.locale.
  language VARCHAR(5) NOT NULL DEFAULT 'en',
  created_at BIGINT NOT NULL DEFAULT (strftime('%s','now')),
  updated_at BIGINT NOT NULL DEFAULT (strftime('%s','now')),
  -- Freeze columns folded in here: SQLite ALTER TABLE has no ADD COLUMN IF NOT EXISTS for 0002 to use.
  is_frozen BOOLEAN NOT NULL DEFAULT 0,
  frozen_at BIGINT NOT NULL DEFAULT 0,
  frozen_by BIGINT NOT NULL DEFAULT 0,
  freeze_reason TEXT NOT NULL DEFAULT ''
);

CREATE TABLE IF NOT EXISTS players (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  steam_id BIGINT UNIQUE NOT NULL,
  name VARCHAR(128) NOT NULL,
  ip_address VARCHAR(45),
  first_seen BIGINT NOT NULL DEFAULT (strftime('%s','now')),
  last_seen BIGINT NOT NULL DEFAULT (strftime('%s','now')),
  total_connections INTEGER NOT NULL DEFAULT 1,
  total_playtime BIGINT NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS bans (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  target_steam_id BIGINT NOT NULL,
  target_name VARCHAR(128) NOT NULL,
  target_ip VARCHAR(45),
  admin_steam_id BIGINT NOT NULL DEFAULT 0,
  admin_name VARCHAR(128) NOT NULL,
  reason TEXT NOT NULL,
  created_at BIGINT NOT NULL DEFAULT (strftime('%s','now')),
  expires_at BIGINT NOT NULL DEFAULT 0,
  duration BIGINT NOT NULL DEFAULT 0,
  is_active BOOLEAN NOT NULL DEFAULT 1,
  removed_at BIGINT DEFAULT 0,
  removed_by BIGINT DEFAULT 0,
  removed_reason TEXT DEFAULT ''
);

CREATE TABLE IF NOT EXISTS voice_mutes (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  target_steam_id BIGINT NOT NULL,
  target_name VARCHAR(128) NOT NULL,
  admin_steam_id BIGINT NOT NULL DEFAULT 0,
  admin_name VARCHAR(128) NOT NULL,
  reason TEXT NOT NULL,
  created_at BIGINT NOT NULL DEFAULT (strftime('%s','now')),
  expires_at BIGINT NOT NULL DEFAULT 0,
  duration BIGINT NOT NULL DEFAULT 0,
  is_active BOOLEAN NOT NULL DEFAULT 1,
  removed_at BIGINT DEFAULT 0,
  removed_by BIGINT DEFAULT 0,
  removed_reason TEXT DEFAULT ''
);

CREATE TABLE IF NOT EXISTS text_mutes (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  target_steam_id BIGINT NOT NULL,
  target_name VARCHAR(128) NOT NULL,
  admin_steam_id BIGINT NOT NULL DEFAULT 0,
  admin_name VARCHAR(128) NOT NULL,
  reason TEXT NOT NULL,
  created_at BIGINT NOT NULL DEFAULT (strftime('%s','now')),
  expires_at BIGINT NOT NULL DEFAULT 0,
  duration BIGINT NOT NULL DEFAULT 0,
  is_active BOOLEAN NOT NULL DEFAULT 1,
  removed_at BIGINT DEFAULT 0,
  removed_by BIGINT DEFAULT 0,
  removed_reason TEXT DEFAULT ''
);

CREATE TABLE IF NOT EXISTS warnings (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  target_steam_id BIGINT NOT NULL,
  target_name VARCHAR(128) NOT NULL,
  admin_steam_id BIGINT NOT NULL DEFAULT 0,
  admin_name VARCHAR(128) NOT NULL,
  reason TEXT NOT NULL,
  created_at BIGINT NOT NULL DEFAULT (strftime('%s','now')),
  is_active BOOLEAN NOT NULL DEFAULT 1,
  expires_at BIGINT DEFAULT 0
);

CREATE INDEX IF NOT EXISTS idx_admins_steam_id ON admins(steam_id);

CREATE INDEX IF NOT EXISTS idx_players_steam_id ON players(steam_id);
CREATE INDEX IF NOT EXISTS idx_players_last_seen ON players(last_seen DESC);

CREATE INDEX IF NOT EXISTS idx_bans_target_steam_id ON bans(target_steam_id);
CREATE INDEX IF NOT EXISTS idx_bans_target_ip ON bans(target_ip);
CREATE INDEX IF NOT EXISTS idx_bans_is_active ON bans(is_active);
CREATE INDEX IF NOT EXISTS idx_bans_expires_at ON bans(expires_at);
CREATE INDEX IF NOT EXISTS idx_bans_active_lookup ON bans(target_steam_id, is_active, expires_at);

CREATE INDEX IF NOT EXISTS idx_voice_mutes_target_steam_id ON voice_mutes(target_steam_id);
CREATE INDEX IF NOT EXISTS idx_voice_mutes_is_active ON voice_mutes(is_active);
CREATE INDEX IF NOT EXISTS idx_voice_mutes_active_lookup ON voice_mutes(target_steam_id, is_active, expires_at);

CREATE INDEX IF NOT EXISTS idx_text_mutes_target_steam_id ON text_mutes(target_steam_id);
CREATE INDEX IF NOT EXISTS idx_text_mutes_is_active ON text_mutes(is_active);
CREATE INDEX IF NOT EXISTS idx_text_mutes_active_lookup ON text_mutes(target_steam_id, is_active, expires_at);

CREATE INDEX IF NOT EXISTS idx_warnings_target_steam_id ON warnings(target_steam_id);
CREATE INDEX IF NOT EXISTS idx_warnings_is_active ON warnings(is_active);

INSERT OR IGNORE INTO admin_groups (name, flags, immunity, inherits, chat_prefix, prefix_color, name_color, message_color)
VALUES ('super_admin', 'z', 100, '[]', '[ROOT]', 'red', 'lightblue', 'default');
