#pragma once

#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <string>
#include <unordered_set>
#include <utility>

namespace AdminSystem::Database
{

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
 * PostgreSQL access layer. Holds ONE long-lived connection (opened in Initialize, reused for every
 * query) and prepares each named statement once. Main-thread-only; the mutex is the single sanctioned
 * exception in the codebase, kept so a future off-thread query path stays safe. A connection dropped
 * while idle is reopened lazily on the next query (the failing query surfaces its error; the one after
 * it reconnects).
 */
class Database
{
public:
    Database() = default;

    /** Open the connection. Returns false if it cannot be established. */
    bool Initialize(const DatabaseConfig& config);

    /** Close the connection and forget every prepared statement. */
    void CloseConnection();

    /** True while the connection is open. */
    bool IsConnected() const;

    /** Run a constant SQL statement (no parameters). Throws on failure. */
    pqxx::result Execute(const std::string& query);

    /** Run a named prepared statement, preparing it once on first use. Throws on failure. */
    template <typename... Args>
    pqxx::result ExecutePrepared(const std::string& name, const std::string& query, Args&&... params);

    /** Run `fn(connection)` against the live connection under the lock (reopening if needed). For
     *  multi-statement work like the migration runner that manages its own transactions. Throws if the
     *  connection cannot be opened; exceptions thrown by `fn` propagate. */
    template <typename Fn>
    auto WithConnection(Fn&& fn) -> decltype(fn(std::declval<pqxx::connection&>()));

private:
    /** Open `_connection` if it is null/closed, clearing the prepared-statement cache. Assumes the
     *  mutex is held. Throws std::runtime_error if the connection cannot be opened. */
    void EnsureOpen();
    /** Drop the connection and the prepared-statement cache. Assumes the mutex is held. */
    void Reset();

    std::string _connectionString;
    std::unique_ptr<pqxx::connection> _connection;
    std::unordered_set<std::string> _prepared;
    std::mutex _mutex;
};

template <typename... Args>
pqxx::result Database::ExecutePrepared(const std::string& name, const std::string& query, Args&&... params)
{
    std::lock_guard<std::mutex> lock(_mutex);
    EnsureOpen();

    if (!_prepared.contains(name))
    {
        _connection->prepare(name, query);
        _prepared.insert(name);
    }

    pqxx::work txn(*_connection);
    pqxx::result result = txn.exec_prepared(name, std::forward<Args>(params)...);
    txn.commit();
    return result;
}

template <typename Fn>
auto Database::WithConnection(Fn&& fn) -> decltype(fn(std::declval<pqxx::connection&>()))
{
    std::lock_guard<std::mutex> lock(_mutex);
    EnsureOpen();
    return fn(*_connection);
}

}  // namespace AdminSystem::Database
