#pragma once

#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <string>

namespace AdminSystem::Database
{
using namespace CS2Kit::Core;

/** PostgreSQL connection parameters loaded from the "database" section of settings.json.
 *  Field names match the JSON keys so they auto-deserialize via CS2Kit::Utils::Json. */
struct DatabaseConfig
{
    std::string host = "localhost";
    int port = 5432;
    std::string database = "cs2_server";
    std::string username = "admin_system";
    std::string password;
    std::string sslMode = "prefer";

    std::string GetConnectionString() const;
};

/**
 * PostgreSQL database access layer. Thread-safe (mutex-protected) since
 * future async queries may run off the game thread.
 */
class Database
{
public:
    Database() = default;

    /** Initializes the database connection. */
    bool Initialize(const DatabaseConfig& config);

    void CloseConnection();
    bool IsConnected() const;
    std::unique_ptr<pqxx::connection> GetConnection();

    /** Executes a simple query. */
    pqxx::result Execute(const std::string& query);

    /** Executes a prepared statement with parameters. */
    template <typename... Args>
    pqxx::result ExecutePrepared(const std::string& name, const std::string& query, Args&&... params);

private:
    std::string _connectionString;
    bool _initialized = false;
    std::mutex _mutex;
};

template <typename... Args>
pqxx::result Database::ExecutePrepared(const std::string& name, const std::string& query, Args&&... params)
{
    auto conn = GetConnection();
    if (!conn)
    {
        throw std::runtime_error("Failed to get database connection");
    }

    pqxx::work txn(*conn);
    conn->prepare(name, query);
    pqxx::result result = txn.exec_prepared(name, std::forward<Args>(params)...);
    txn.commit();
    return result;
}

}  // namespace AdminSystem::Database
