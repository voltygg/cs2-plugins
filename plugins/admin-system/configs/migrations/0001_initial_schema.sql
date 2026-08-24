-- 0001 Initial schema
-- Applied automatically on plugin load by VoltMod::Database / AdminSystem::Database::RunMigrations.
-- Forward-only: never edit an applied migration; add a new NNNN_*.sql instead.

-- ------- TABLES -----
CREATE TABLE IF NOT EXISTS admin_groups (
  id BIGSERIAL PRIMARY KEY,
  name VARCHAR(64) UNIQUE NOT NULL,
  flags VARCHAR(64) NOT NULL DEFAULT '',
  immunity INTEGER NOT NULL DEFAULT 0,
  inherits TEXT[] DEFAULT ARRAY[]::TEXT[],
  chat_prefix VARCHAR(64) NOT NULL DEFAULT '',
  prefix_color VARCHAR(32) NOT NULL DEFAULT '',
  name_color VARCHAR(32) NOT NULL DEFAULT '',
  message_color VARCHAR(32) NOT NULL DEFAULT '',
  created_at BIGINT NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW())::BIGINT,
  updated_at BIGINT NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW())::BIGINT
);

CREATE TABLE IF NOT EXISTS admins (
  id BIGSERIAL PRIMARY KEY,
  steam_id BIGINT UNIQUE NOT NULL,
  name VARCHAR(128) NOT NULL,
  groups TEXT[] DEFAULT ARRAY[]::TEXT[],
  flags VARCHAR(64) NOT NULL DEFAULT '',
  immunity INTEGER NOT NULL DEFAULT 0,
  -- Per-admin chat overrides. Empty colors fall back to the admin's group; display_prefix lets an
  -- admin hide their group prefix without losing their group's coloring.
  display_prefix BOOLEAN NOT NULL DEFAULT TRUE,
  name_color VARCHAR(32) NOT NULL DEFAULT '',
  message_color VARCHAR(32) NOT NULL DEFAULT '',
  -- Per-admin language for the in-game admin panel. Defaults to English regardless of plugin.locale.
  language VARCHAR(5) NOT NULL DEFAULT 'en',
  created_at BIGINT NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW())::BIGINT,
  updated_at BIGINT NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW())::BIGINT
);

CREATE TABLE IF NOT EXISTS players (
  id BIGSERIAL PRIMARY KEY,
  steam_id BIGINT UNIQUE NOT NULL,
  name VARCHAR(128) NOT NULL,
  ip_address VARCHAR(45),
  first_seen BIGINT NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW())::BIGINT,
  last_seen BIGINT NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW())::BIGINT,
  total_connections INTEGER NOT NULL DEFAULT 1,
  total_playtime BIGINT NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS bans (
  id BIGSERIAL PRIMARY KEY,
  target_steam_id BIGINT NOT NULL,
  target_name VARCHAR(128) NOT NULL,
  target_ip VARCHAR(45),
  admin_steam_id BIGINT NOT NULL DEFAULT 0,
  admin_name VARCHAR(128) NOT NULL,
  reason TEXT NOT NULL,
  created_at BIGINT NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW())::BIGINT,
  expires_at BIGINT NOT NULL DEFAULT 0,
  duration BIGINT NOT NULL DEFAULT 0,
  is_active BOOLEAN NOT NULL DEFAULT TRUE,
  removed_at BIGINT DEFAULT 0,
  removed_by BIGINT DEFAULT 0,
  removed_reason TEXT DEFAULT ''
);

CREATE TABLE IF NOT EXISTS voice_mutes (
  id BIGSERIAL PRIMARY KEY,
  target_steam_id BIGINT NOT NULL,
  target_name VARCHAR(128) NOT NULL,
  admin_steam_id BIGINT NOT NULL DEFAULT 0,
  admin_name VARCHAR(128) NOT NULL,
  reason TEXT NOT NULL,
  created_at BIGINT NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW())::BIGINT,
  expires_at BIGINT NOT NULL DEFAULT 0,
  duration BIGINT NOT NULL DEFAULT 0,
  is_active BOOLEAN NOT NULL DEFAULT TRUE,
  removed_at BIGINT DEFAULT 0,
  removed_by BIGINT DEFAULT 0,
  removed_reason TEXT DEFAULT ''
);

CREATE TABLE IF NOT EXISTS text_mutes (
  id BIGSERIAL PRIMARY KEY,
  target_steam_id BIGINT NOT NULL,
  target_name VARCHAR(128) NOT NULL,
  admin_steam_id BIGINT NOT NULL DEFAULT 0,
  admin_name VARCHAR(128) NOT NULL,
  reason TEXT NOT NULL,
  created_at BIGINT NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW())::BIGINT,
  expires_at BIGINT NOT NULL DEFAULT 0,
  duration BIGINT NOT NULL DEFAULT 0,
  is_active BOOLEAN NOT NULL DEFAULT TRUE,
  removed_at BIGINT DEFAULT 0,
  removed_by BIGINT DEFAULT 0,
  removed_reason TEXT DEFAULT ''
);

CREATE TABLE IF NOT EXISTS warnings (
  id BIGSERIAL PRIMARY KEY,
  target_steam_id BIGINT NOT NULL,
  target_name VARCHAR(128) NOT NULL,
  admin_steam_id BIGINT NOT NULL DEFAULT 0,
  admin_name VARCHAR(128) NOT NULL,
  reason TEXT NOT NULL,
  created_at BIGINT NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW())::BIGINT,
  is_active BOOLEAN NOT NULL DEFAULT TRUE,
  expires_at BIGINT DEFAULT 0
);

-- ------- INDEXES -----
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

-- ------- DEFAULT DATA -----
INSERT INTO admin_groups (name, flags, immunity, inherits, chat_prefix, prefix_color, name_color, message_color)
VALUES ('super_admin', 'z', 100, ARRAY[]::TEXT[], '[ROOT]', 'red', 'lightblue', 'default')
ON CONFLICT (name) DO NOTHING;
