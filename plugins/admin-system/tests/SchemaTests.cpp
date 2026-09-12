// The schema, the migrations and the generated specs are three views of one thing. Nothing
// else checks that a rendered migration re-runs, or that a spec names a column that exists.

#include "Database/Tables/Schema.hpp"

#include <VoltMod/Core/File.hpp>
#include <VoltMod/Core/Scheduler.hpp>
#include <VoltMod/Database/Api.hpp>
#include <doctest/doctest.h>
#include <filesystem>
#include <string>
#include <vector>

namespace Tables = AdminSystem::Database::Tables;
using VoltMod::Database;
using VoltMod::RunMigrations;
using VoltMod::Scheduler;

static constexpr const char* Drivers[] = {"postgres", "mariadb", "sqlite"};

static std::filesystem::path MigrationsDir()
{
    return std::filesystem::path(ADMIN_SYSTEM_DIR) / "configs" / "migrations";
}

/** doctest evaluates the message eagerly, so this has to be safe on a successful result. */
template <class T>
static std::string ErrorText(const VoltMod::Result<T>& result)
{
    return result ? std::string{} : result.error().Detail;
}

static std::string ReadMigration(const std::string& driver)
{
    auto sql = VoltMod::ReadAllText((MigrationsDir() / driver / "0001_initial_schema.sql").string());
    REQUIRE_MESSAGE(sql.has_value(), "missing migration for ", driver);
    return *sql;
}

TEST_CASE("Every rendered migration resolves all of its placeholders")
{
    for (const char* driver : Drivers)
    {
        // Guards the shipped file, not the renderer: only a hand edit leaves a '@' behind.
        CHECK_MESSAGE(ReadMigration(driver).find('@') == std::string::npos, "unrendered token in ", driver);
    }
}

TEST_CASE("Every rendered migration is safe to re-run")
{
    for (const char* driver : Drivers)
    {
        const auto statements = VoltMod::SplitStatements(ReadMigration(driver));
        CHECK(statements.size() == 34);  // 11 tables, 22 indexes, 1 seed row
        for (const auto& statement : statements)
        {
            const bool idempotent = statement.starts_with("CREATE TABLE IF NOT EXISTS") ||
                                    statement.starts_with("CREATE INDEX IF NOT EXISTS") ||
                                    statement.starts_with("INSERT");
            CHECK_MESSAGE(idempotent, driver, ": ", statement.substr(0, 60));
        }
    }
}

/** Runs @p body against a fresh in-memory database with the plugin's migrations applied. */
template <class Body>
static void WithMigratedDb(Body body)
{
    Scheduler scheduler;
    Database db(scheduler);
    REQUIRE(db.Start({.driver = "sqlite", .path = ":memory:"}));

    auto migration = RunMigrations(db, MigrationsDir().string());
    REQUIRE(migration.Success);
    body(db, migration);
}

TEST_CASE("The migrations build the schema the generated specs describe")
{
    WithMigratedDb([](Database& db, const auto& migration) {
        CHECK(migration.Applied == 1);
        CHECK(migration.CurrentVersion == 1);

        // SQLite raises "no such column" when a spec and the schema disagree, so selecting
        // every column is the whole check. Matching no rows keeps it cheap.
        auto probe = db.Run("select-all-columns", [](auto& conn) {
            const auto count = [&conn](auto table) {
                int rows = 0;
                for (const auto& row : conn(sqlpp::select(sqlpp::all_of(table)).from(table).where(table.id == 0)))
                {
                    (void)row;
                    ++rows;
                }
                return rows;
            };
            return count(Tables::AdminGroups{}) + count(Tables::Admins{}) + count(Tables::Players{}) +
                   count(Tables::Bans{}) + count(Tables::VoiceMutes{}) + count(Tables::TextMutes{}) +
                   count(Tables::Warnings{}) + count(Tables::Servers{}) + count(Tables::AdminServerGroups{}) +
                   count(Tables::AdminActivity{}) + count(Tables::PlayerReports{});
        });
        REQUIRE_MESSAGE(probe.has_value(), ErrorText(probe));
        CHECK(*probe == 0);

        CHECK(db.Run("seeded-group", [](auto& conn) {
                    const Tables::AdminGroups t;
                    int found = 0;
                    for (const auto& row : conn(sqlpp::select(t.name).from(t).where(t.name == "super_admin")))
                    {
                        (void)row;
                        ++found;
                    }
                    return found;
                }).value_or(0) == 1);
    });
}

TEST_CASE("An insert that supplies only the required columns is accepted")
{
    WithMigratedDb([](Database& db, const auto&) {
        // Every omitted column is one the generator gave a default. If it got one wrong,
        // sqlpp23 will not compile this or SQLite rejects the row.
        auto id = db.Run("minimal-ban", [](auto& conn) {
            const Tables::Bans t;
            return VoltMod::Insert(
                conn,
                sqlpp::insert_into(t).set(t.targetSteamId = 76561198000000000LL, t.targetName = "someone",
                                          t.adminName = "console", t.reason = "testing"),
                "bans");
        });
        REQUIRE_MESSAGE(id.has_value(), ErrorText(id));
        CHECK(*id > 0);
    });
}
