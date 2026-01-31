-- Admin System Database Schema
-- PostgreSQL 18+

-- Create dedicated schema for plugin data
CREATE SCHEMA IF NOT EXISTS admin_system;

-- Set search path
SET search_path TO admin_system;

-- Admin Groups
CREATE TABLE admin_groups (
    id SERIAL PRIMARY KEY,
    name VARCHAR(64) NOT NULL UNIQUE,
    flags VARCHAR(32) NOT NULL DEFAULT '',
    immunity_level INT NOT NULL DEFAULT 0,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Admins
CREATE TABLE admins (
    id SERIAL PRIMARY KEY,
    steam_id BIGINT NOT NULL UNIQUE,
    name VARCHAR(128) NOT NULL,
    flags VARCHAR(32) NOT NULL DEFAULT '',
    immunity_level INT NOT NULL DEFAULT 0,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    created_by BIGINT,
    notes TEXT
);

-- Admin Group Membership
CREATE TABLE admin_group_members (
    admin_id INT NOT NULL REFERENCES admins(id) ON DELETE CASCADE,
    group_id INT NOT NULL REFERENCES admin_groups(id) ON DELETE CASCADE,
    PRIMARY KEY (admin_id, group_id)
);

-- Players (tracking)
CREATE TABLE players (
    id SERIAL PRIMARY KEY,
    steam_id BIGINT NOT NULL UNIQUE,
    name VARCHAR(128) NOT NULL,
    ip_address INET,
    country VARCHAR(2),
    first_seen TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    last_seen TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    total_connections INT NOT NULL DEFAULT 1,
    total_playtime_seconds BIGINT NOT NULL DEFAULT 0
);

CREATE INDEX idx_players_name ON players(name);
CREATE INDEX idx_players_last_seen ON players(last_seen);

-- Bans
CREATE TABLE bans (
    id SERIAL PRIMARY KEY,
    target_steam_id BIGINT NOT NULL,
    target_name VARCHAR(128) NOT NULL,
    target_ip INET,
    admin_steam_id BIGINT,
    admin_name VARCHAR(128) NOT NULL,
    reason TEXT NOT NULL DEFAULT 'No reason given',
    duration_seconds INT,  -- NULL = permanent
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    expires_at TIMESTAMPTZ,  -- NULL = permanent
    unbanned_at TIMESTAMPTZ,
    unbanned_by BIGINT,
    unbanned_reason TEXT,
    is_active BOOLEAN NOT NULL DEFAULT TRUE,
    server_id VARCHAR(64)
);

CREATE INDEX idx_bans_target ON bans(target_steam_id);
CREATE INDEX idx_bans_active ON bans(is_active) WHERE is_active = TRUE;
CREATE INDEX idx_bans_ip ON bans(target_ip) WHERE target_ip IS NOT NULL;

-- Mutes (voice)
CREATE TABLE mutes (
    id SERIAL PRIMARY KEY,
    target_steam_id BIGINT NOT NULL,
    target_name VARCHAR(128) NOT NULL,
    admin_steam_id BIGINT,
    admin_name VARCHAR(128) NOT NULL,
    reason TEXT NOT NULL DEFAULT 'No reason given',
    duration_seconds INT,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    expires_at TIMESTAMPTZ,
    removed_at TIMESTAMPTZ,
    removed_by BIGINT,
    is_active BOOLEAN NOT NULL DEFAULT TRUE,
    server_id VARCHAR(64)
);

CREATE INDEX idx_mutes_target ON mutes(target_steam_id);
CREATE INDEX idx_mutes_active ON mutes(is_active) WHERE is_active = TRUE;

-- Gags (text chat)
CREATE TABLE gags (
    id SERIAL PRIMARY KEY,
    target_steam_id BIGINT NOT NULL,
    target_name VARCHAR(128) NOT NULL,
    admin_steam_id BIGINT,
    admin_name VARCHAR(128) NOT NULL,
    reason TEXT NOT NULL DEFAULT 'No reason given',
    duration_seconds INT,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    expires_at TIMESTAMPTZ,
    removed_at TIMESTAMPTZ,
    removed_by BIGINT,
    is_active BOOLEAN NOT NULL DEFAULT TRUE,
    server_id VARCHAR(64)
);

CREATE INDEX idx_gags_target ON gags(target_steam_id);
CREATE INDEX idx_gags_active ON gags(is_active) WHERE is_active = TRUE;

-- Warnings
CREATE TABLE warnings (
    id SERIAL PRIMARY KEY,
    target_steam_id BIGINT NOT NULL,
    target_name VARCHAR(128) NOT NULL,
    admin_steam_id BIGINT,
    admin_name VARCHAR(128) NOT NULL,
    reason TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    acknowledged_at TIMESTAMPTZ,
    server_id VARCHAR(64)
);

CREATE INDEX idx_warnings_target ON warnings(target_steam_id);

-- Audit Log
CREATE TABLE audit_log (
    id BIGSERIAL PRIMARY KEY,
    action VARCHAR(64) NOT NULL,
    actor_steam_id BIGINT,
    actor_name VARCHAR(128),
    target_steam_id BIGINT,
    target_name VARCHAR(128),
    details JSONB,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    server_id VARCHAR(64),
    ip_address INET
);

CREATE INDEX idx_audit_log_action ON audit_log(action);
CREATE INDEX idx_audit_log_actor ON audit_log(actor_steam_id);
CREATE INDEX idx_audit_log_target ON audit_log(target_steam_id);
CREATE INDEX idx_audit_log_created ON audit_log(created_at);

-- Schema Migrations Tracking
CREATE TABLE migrations (
    id SERIAL PRIMARY KEY,
    version VARCHAR(32) NOT NULL UNIQUE,
    name VARCHAR(128) NOT NULL,
    applied_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Insert initial migration record
INSERT INTO migrations (version, name) VALUES ('001', 'initial_schema');

-- Useful Views
CREATE VIEW active_bans AS
SELECT * FROM bans
WHERE is_active = TRUE
  AND (expires_at IS NULL OR expires_at > NOW());

CREATE VIEW active_mutes AS
SELECT * FROM mutes
WHERE is_active = TRUE
  AND (expires_at IS NULL OR expires_at > NOW());

CREATE VIEW active_gags AS
SELECT * FROM gags
WHERE is_active = TRUE
  AND (expires_at IS NULL OR expires_at > NOW());

-- Default admin groups
INSERT INTO admin_groups (name, flags, immunity_level) VALUES
    ('superadmin', 'z', 100),
    ('admin', 'bcdfopqr', 60),
    ('moderator', 'bcopqr', 40),
    ('vip', 'a', 5);

-- Grant permissions to the application user
GRANT USAGE ON SCHEMA admin_system TO admin_system;
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA admin_system TO admin_system;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA admin_system TO admin_system;
