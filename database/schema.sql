-- CS2 Admin System Database Schema
-- PostgreSQL 16+
-- Schema: public (default)
-- ------- TABLES -----
-- Admin groups table
CREATE TABLE IF NOT EXISTS admin_groups (
  id BIGSERIAL PRIMARY KEY,
  name VARCHAR(64) UNIQUE NOT NULL,
  flags VARCHAR(64) NOT NULL DEFAULT '',
  immunity INTEGER NOT NULL DEFAULT 0,
  inherits TEXT [ ] DEFAULT ARRAY [ ] :: TEXT [ ],
  chat_prefix VARCHAR(64) NOT NULL DEFAULT '',
  prefix_color VARCHAR(32) NOT NULL DEFAULT '',
  name_color VARCHAR(32) NOT NULL DEFAULT '',
  message_color VARCHAR(32) NOT NULL DEFAULT '',
  created_at BIGINT NOT NULL DEFAULT EXTRACT(
    EPOCH
    FROM
      NOW()
  ) :: BIGINT,
  updated_at BIGINT NOT NULL DEFAULT EXTRACT(
    EPOCH
    FROM
      NOW()
  ) :: BIGINT
);

-- Admins table
CREATE TABLE IF NOT EXISTS admins (
  id BIGSERIAL PRIMARY KEY,
  steam_id BIGINT UNIQUE NOT NULL,
  name VARCHAR(128) NOT NULL,
  groups TEXT [ ] DEFAULT ARRAY [ ] :: TEXT [ ],
  flags VARCHAR(64) NOT NULL DEFAULT '',
  immunity INTEGER NOT NULL DEFAULT 0,
  created_at BIGINT NOT NULL DEFAULT EXTRACT(
    EPOCH
    FROM
      NOW()
  ) :: BIGINT,
  updated_at BIGINT NOT NULL DEFAULT EXTRACT(
    EPOCH
    FROM
      NOW()
  ) :: BIGINT
);

-- Players table
CREATE TABLE IF NOT EXISTS players (
  id BIGSERIAL PRIMARY KEY,
  steam_id BIGINT UNIQUE NOT NULL,
  name VARCHAR(128) NOT NULL,
  ip_address VARCHAR(45),
  first_seen BIGINT NOT NULL DEFAULT EXTRACT(
    EPOCH
    FROM
      NOW()
  ) :: BIGINT,
  last_seen BIGINT NOT NULL DEFAULT EXTRACT(
    EPOCH
    FROM
      NOW()
  ) :: BIGINT,
  total_connections INTEGER NOT NULL DEFAULT 1,
  total_playtime BIGINT NOT NULL DEFAULT 0
);

-- Bans table
CREATE TABLE IF NOT EXISTS bans (
  id BIGSERIAL PRIMARY KEY,
  target_steam_id BIGINT NOT NULL,
  target_name VARCHAR(128) NOT NULL,
  target_ip VARCHAR(45),
  admin_steam_id BIGINT NOT NULL DEFAULT 0,
  admin_name VARCHAR(128) NOT NULL,
  reason TEXT NOT NULL,
  created_at BIGINT NOT NULL DEFAULT EXTRACT(
    EPOCH
    FROM
      NOW()
  ) :: BIGINT,
  expires_at BIGINT NOT NULL DEFAULT 0,
  duration BIGINT NOT NULL DEFAULT 0,
  is_active BOOLEAN NOT NULL DEFAULT TRUE,
  removed_at BIGINT DEFAULT 0,
  removed_by BIGINT DEFAULT 0,
  removed_reason TEXT DEFAULT ''
);

-- Voice mutes table (in-game voice chat blocks)
CREATE TABLE IF NOT EXISTS voice_mutes (
  id BIGSERIAL PRIMARY KEY,
  target_steam_id BIGINT NOT NULL,
  target_name VARCHAR(128) NOT NULL,
  admin_steam_id BIGINT NOT NULL DEFAULT 0,
  admin_name VARCHAR(128) NOT NULL,
  reason TEXT NOT NULL,
  created_at BIGINT NOT NULL DEFAULT EXTRACT(
    EPOCH
    FROM
      NOW()
  ) :: BIGINT,
  expires_at BIGINT NOT NULL DEFAULT 0,
  duration BIGINT NOT NULL DEFAULT 0,
  is_active BOOLEAN NOT NULL DEFAULT TRUE,
  removed_at BIGINT DEFAULT 0,
  removed_by BIGINT DEFAULT 0,
  removed_reason TEXT DEFAULT ''
);

-- Text mutes table (text chat blocks)
CREATE TABLE IF NOT EXISTS text_mutes (
  id BIGSERIAL PRIMARY KEY,
  target_steam_id BIGINT NOT NULL,
  target_name VARCHAR(128) NOT NULL,
  admin_steam_id BIGINT NOT NULL DEFAULT 0,
  admin_name VARCHAR(128) NOT NULL,
  reason TEXT NOT NULL,
  created_at BIGINT NOT NULL DEFAULT EXTRACT(
    EPOCH
    FROM
      NOW()
  ) :: BIGINT,
  expires_at BIGINT NOT NULL DEFAULT 0,
  duration BIGINT NOT NULL DEFAULT 0,
  is_active BOOLEAN NOT NULL DEFAULT TRUE,
  removed_at BIGINT DEFAULT 0,
  removed_by BIGINT DEFAULT 0,
  removed_reason TEXT DEFAULT ''
);

-- Warnings table
CREATE TABLE IF NOT EXISTS warnings (
  id BIGSERIAL PRIMARY KEY,
  target_steam_id BIGINT NOT NULL,
  target_name VARCHAR(128) NOT NULL,
  admin_steam_id BIGINT NOT NULL DEFAULT 0,
  admin_name VARCHAR(128) NOT NULL,
  reason TEXT NOT NULL,
  created_at BIGINT NOT NULL DEFAULT EXTRACT(
    EPOCH
    FROM
      NOW()
  ) :: BIGINT,
  is_active BOOLEAN NOT NULL DEFAULT TRUE,
  expires_at BIGINT DEFAULT 0
);

-- Audit log table
CREATE TABLE IF NOT EXISTS audit_log (
  id BIGSERIAL PRIMARY KEY,
  admin_steam_id BIGINT NOT NULL DEFAULT 0,
  admin_name VARCHAR(128) NOT NULL,
  action VARCHAR(64) NOT NULL,
  target_steam_id BIGINT DEFAULT 0,
  target_name VARCHAR(128) DEFAULT '',
  details TEXT,
  created_at BIGINT NOT NULL DEFAULT EXTRACT(
    EPOCH
    FROM
      NOW()
  ) :: BIGINT
);

-- Schema migrations table (for tracking schema versions)
CREATE TABLE IF NOT EXISTS migrations (
  id SERIAL PRIMARY KEY,
  version INTEGER UNIQUE NOT NULL,
  description VARCHAR(255) NOT NULL,
  applied_at BIGINT NOT NULL DEFAULT EXTRACT(
    EPOCH
    FROM
      NOW()
  ) :: BIGINT
);

-- ------- INDEXES -----
-- Admins indexes
CREATE INDEX IF NOT EXISTS idx_admins_steam_id ON admins(steam_id);

-- Players indexes
CREATE INDEX IF NOT EXISTS idx_players_steam_id ON players(steam_id);

CREATE INDEX IF NOT EXISTS idx_players_last_seen ON players(last_seen DESC);

-- Bans indexes
CREATE INDEX IF NOT EXISTS idx_bans_target_steam_id ON bans(target_steam_id);

CREATE INDEX IF NOT EXISTS idx_bans_target_ip ON bans(target_ip);

CREATE INDEX IF NOT EXISTS idx_bans_is_active ON bans(is_active);

CREATE INDEX IF NOT EXISTS idx_bans_expires_at ON bans(expires_at);

CREATE INDEX IF NOT EXISTS idx_bans_active_lookup ON bans(target_steam_id, is_active, expires_at);

-- Voice mutes indexes
CREATE INDEX IF NOT EXISTS idx_voice_mutes_target_steam_id ON voice_mutes(target_steam_id);

CREATE INDEX IF NOT EXISTS idx_voice_mutes_is_active ON voice_mutes(is_active);

CREATE INDEX IF NOT EXISTS idx_voice_mutes_active_lookup ON voice_mutes(target_steam_id, is_active, expires_at);

-- Text mutes indexes
CREATE INDEX IF NOT EXISTS idx_text_mutes_target_steam_id ON text_mutes(target_steam_id);

CREATE INDEX IF NOT EXISTS idx_text_mutes_is_active ON text_mutes(is_active);

CREATE INDEX IF NOT EXISTS idx_text_mutes_active_lookup ON text_mutes(target_steam_id, is_active, expires_at);

-- Warnings indexes
CREATE INDEX IF NOT EXISTS idx_warnings_target_steam_id ON warnings(target_steam_id);

CREATE INDEX IF NOT EXISTS idx_warnings_is_active ON warnings(is_active);

-- Audit log indexes
CREATE INDEX IF NOT EXISTS idx_audit_log_admin_steam_id ON audit_log(admin_steam_id);

CREATE INDEX IF NOT EXISTS idx_audit_log_target_steam_id ON audit_log(target_steam_id);

CREATE INDEX IF NOT EXISTS idx_audit_log_created_at ON audit_log(created_at DESC);

-- ------- DEFAULT DATA -----
-- Insert default superadmin group
INSERT INTO
  admin_groups (name, flags, immunity, inherits, chat_prefix, prefix_color, name_color, message_color)
VALUES
  ('superadmin', 'z', 100, ARRAY [ ] :: TEXT [ ], '[ROOT]', 'red', 'lightblue', 'default') ON CONFLICT (name) DO NOTHING;

-- Insert default moderator group
INSERT INTO
  admin_groups (name, flags, immunity, inherits, chat_prefix, prefix_color, name_color, message_color)
VALUES
  ('moderator', 'bci', 50, ARRAY [ ] :: TEXT [ ], '[MOD]', 'green', 'default', 'default') ON CONFLICT (name) DO NOTHING;

-- Insert schema version
INSERT INTO
  migrations (version, description)
VALUES
  (1, 'Initial schema creation') ON CONFLICT (version) DO NOTHING;
