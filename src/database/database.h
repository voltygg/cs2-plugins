#pragma once

#include <pqxx/pqxx>
#include <memory>
#include <string>
#include <mutex>

namespace database {

/**
 * @brief Database configuration
 */
struct DatabaseConfig
{
    std::string host = "localhost";
    int port = 5432;
    std::string database = "cs2_server";
    std::string username = "admin_system";
    std::string password = "";
    std::string schema = "admin_system";
    int pool_size = 4;
    std::string ssl_mode = "prefer";

    /**
     * @brief Build PostgreSQL connection string
     * @return Connection string for libpqxx
     */
    std::string GetConnectionString() const;
};

/**
 * @brief Database connection manager with connection pooling
 */
class Database
{
public:
    /**
     * @brief Initialize database connection
     * @param config Database configuration
     * @return true if successful
     */
    bool Initialize(const DatabaseConfig& config);

    /**
     * @brief Shutdown database connection
     */
    void Shutdown();

    /**
     * @brief Check if database is connected
     * @return true if connected
     */
    bool IsConnected() const;

    /**
     * @brief Get a database connection (thread-safe)
     * @return Unique pointer to connection
     */
    std::unique_ptr<pqxx::connection> GetConnection();

    /**
     * @brief Execute a query synchronously
     * @param query SQL query
     * @return Query result
     */
    pqxx::result Execute(const std::string& query);

    /**
     * @brief Execute a query with parameters synchronously
     * @param name Prepared statement name
     * @param query SQL query
     * @param params Query parameters
     * @return Query result
     */
    template<typename... Args>
    pqxx::result ExecutePrepared(const std::string& name, const std::string& query, Args&&... params);

    /**
     * @brief Get singleton instance
     * @return Database instance
     */
    static Database& Instance();

private:
    Database() = default;
    ~Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    DatabaseConfig m_config;
    std::string m_connectionString;
    bool m_initialized = false;
    std::mutex m_mutex;
};

// Template implementation
template<typename... Args>
pqxx::result Database::ExecutePrepared(const std::string& name, const std::string& query, Args&&... params)
{
    auto conn = GetConnection();
    if (!conn)
    {
        throw std::runtime_error("Failed to get database connection");
    }

    pqxx::work txn(*conn);

    // Prepare statement if not already prepared
    conn->prepare(name, query);

    // Execute prepared statement
    pqxx::result result = txn.exec_prepared(name, std::forward<Args>(params)...);
    txn.commit();

    return result;
}

} // namespace database
