-- 0003 Player reports (in-game !r / !report; triaged by the upstream website)
-- Applied by VoltMod::RunMigrations. Add a new numbered file for later changes.

-- The game server inserts through created_at. The website owns later triage columns.
CREATE TABLE IF NOT EXISTS player_reports (
  id BIGSERIAL PRIMARY KEY,
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
  created_at BIGINT NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW())::BIGINT,
  -- Website-owned triage columns; the plugin never writes these.
  status VARCHAR(16) NOT NULL DEFAULT 'open',  -- open | reviewing | resolved | rejected | duplicate
  handled_by BIGINT NOT NULL DEFAULT 0,        -- admin steam id that closed it
  handled_at BIGINT NOT NULL DEFAULT 0,
  resolution TEXT NOT NULL DEFAULT ''
);

-- Website queue: newest open reports first.
CREATE INDEX IF NOT EXISTS idx_player_reports_status_time ON player_reports(status, created_at DESC);
-- "every report against this player" - the main triage drill-down.
CREATE INDEX IF NOT EXISTS idx_player_reports_target_time ON player_reports(target_steam_id, created_at DESC);
-- Reporter history / false-report abuse detection.
CREATE INDEX IF NOT EXISTS idx_player_reports_reporter_time ON player_reports(reporter_steam_id, created_at DESC);
-- Per-server dashboards and filtering.
CREATE INDEX IF NOT EXISTS idx_player_reports_server_time ON player_reports(server_tag, created_at DESC);
