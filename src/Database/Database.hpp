#pragma once

#include <CS2Kit/Core/Singleton.hpp>

#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <string>

namespace AdminSystem::Database
{
using namespace CS2Kit::Core;

/** PostgreSQL connection parameters loaded from the "database" section of settings.json. */
struct DatabaseConfig
{
    std::string Host = "localhost";
    int Port = 5432;
    std::string DatabaseName = "cs2_server";
    std::string Username = "admin_system";
    std::string Password;
    std::string Schema = "admin_system";
    std::string SslMode = "prefer";

    std::string GetConnectionString() const;
};

/**
 * PostgreSQL database access layer. Thread-safe (mutex-protected) since
 * future async queries may run off the game thread.
 */
class Database : public Singleton<Database>
{
public:
    explicit Database(Token) {}

    bool Initialize(const DatabaseConfig& config);
    void Shutdown();
    bool IsConnected() const;
    std::unique_ptr<pqxx::connection> GetConnection();
    pqxx::result Execute(const std::string& query);

    template <typename... Args>
    pqxx::result ExecutePrepared(const std::string& name, const std::string& query, Args&&... params);

private:
    DatabaseConfig _config;
    std::string _connectionString;
    bool _initialized = false;
    std::mutex _mutex;
};

template <typename... Args>
pqxx::result Database::ExecutePrepared(const std::string& name, const std::string& query, Args&&... params)
{
    auto conn = GetConnection();
    if (!conn)
        throw std::runtime_error("Failed to get database connection");

    pqxx::work txn(*conn);
    conn->prepare(name, query);
    pqxx::result result = txn.exec_prepared(name, std::forward<Args>(params)...);
    txn.commit();
    return result;
}

}  // namespace AdminSystem::Database
