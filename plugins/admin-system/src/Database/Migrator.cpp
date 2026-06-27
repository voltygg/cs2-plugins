#include "Migrator.hpp"

#include "Database.hpp"

#include <CS2Kit/Utils/Log.hpp>
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace AdminSystem::Database
{

namespace Log = CS2Kit::Utils::Log;

namespace
{

constexpr int AdvisoryLockKey = 727274;  // arbitrary constant; isolates concurrent plugin loads

struct Migration
{
    int Version;
    std::string Name;
    fs::path Path;
};

std::optional<int> ParseLeadingVersion(const std::string& filename)
{
    int version = 0;
    const char* begin = filename.data();
    const char* end = begin + filename.size();
    auto [ptr, ec] = std::from_chars(begin, end, version);
    if (ec != std::errc{} || ptr == begin)
        return std::nullopt;
    return version;
}

std::string ReadFile(const fs::path& path)
{
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

}  // namespace

bool RunMigrations(Database& db, const std::string& dir)
{
    std::error_code ec;
    if (!fs::exists(dir, ec))
    {
        Log::Warn("Migrations directory not found ({}); skipping schema setup.", dir);
        return true;
    }

    std::vector<Migration> migrations;
    for (const auto& entry : fs::directory_iterator(dir, ec))
    {
        if (!entry.is_regular_file())
            continue;
        std::string name = entry.path().filename().string();
        if (!name.ends_with(".sql"))
            continue;
        auto version = ParseLeadingVersion(name);
        if (!version)  // ignore stray files without a leading version (e.g. *.sql.bak)
            continue;
        migrations.push_back({*version, name, entry.path()});
    }
    std::sort(migrations.begin(), migrations.end(),
              [](const Migration& a, const Migration& b) { return a.Version < b.Version; });

    try
    {
        return db.WithConnection([&](pqxx::connection& conn) -> bool {
            {
                pqxx::work txn(conn);
                txn.exec(
                    "CREATE TABLE IF NOT EXISTS schema_migrations ("
                    "version INTEGER PRIMARY KEY, "
                    "name TEXT NOT NULL, "
                    "applied_at BIGINT NOT NULL DEFAULT EXTRACT(EPOCH FROM NOW())::BIGINT)");
                txn.commit();
            }

            // Session-level advisory lock held across the per-file transactions below; auto-released if
            // the connection drops. Serializes two plugin loads that race on the same database.
            {
                pqxx::work txn(conn);
                txn.exec("SELECT pg_advisory_lock(" + std::to_string(AdvisoryLockKey) + ")");
                txn.commit();
            }

            int current = 0;
            {
                pqxx::work txn(conn);
                pqxx::result r = txn.exec("SELECT COALESCE(MAX(version), 0) FROM schema_migrations");
                current = r[0][0].as<int>();
                txn.commit();
            }

            int applied = 0;
            bool ok = true;
            for (const Migration& m : migrations)
            {
                if (m.Version <= current)
                    continue;
                try
                {
                    pqxx::work txn(conn);
                    txn.exec(ReadFile(m.Path));
                    txn.exec("INSERT INTO schema_migrations (version, name) VALUES ($1, $2)",
                             pqxx::params{m.Version, m.Name});
                    txn.commit();
                    ++applied;
                    Log::Info("Applied migration {} ({}).", m.Version, m.Name);
                }
                catch (const std::exception& e)
                {
                    Log::Error("Migration {} ({}) failed: {}", m.Version, m.Name, e.what());
                    ok = false;
                    break;
                }
            }

            {
                pqxx::work txn(conn);
                txn.exec("SELECT pg_advisory_unlock(" + std::to_string(AdvisoryLockKey) + ")");
                txn.commit();
            }

            if (applied > 0)
                Log::Info("Database schema up to date ({} migration(s) applied).", applied);
            return ok;
        });
    }
    catch (const std::exception& e)
    {
        Log::Error("Migration runner failed: {}", e.what());
        return false;
    }
}

}  // namespace AdminSystem::Database
