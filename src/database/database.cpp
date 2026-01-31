#include "database.h"
#include <format>
#include <stdexcept>

namespace database {

std::string DatabaseConfig::GetConnectionString() const
{
    return std::format(
        "host={} port={} dbname={} user={} password={} sslmode={}",
        host, port, database, username, password, ssl_mode
    );
}

bool Database::Initialize(const DatabaseConfig& config)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized)
    {
        return true;
    }

    try
    {
        m_config = config;
        m_connectionString = config.GetConnectionString();

        // Test connection
        auto conn = std::make_unique<pqxx::connection>(m_connectionString);
        if (!conn->is_open())
        {
            return false;
        }

        // Set schema search path
        pqxx::work txn(*conn);
        txn.exec(std::format("SET search_path TO {}", config.schema));
        txn.commit();

        m_initialized = true;
        return true;
    }
    catch (const std::exception& e)
    {
        // Log error (TODO: use logging system)
        return false;
    }
}

void Database::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_initialized = false;
}

bool Database::IsConnected() const
{
    return m_initialized;
}

std::unique_ptr<pqxx::connection> Database::GetConnection()
{
    if (!m_initialized)
    {
        return nullptr;
    }

    try
    {
        auto conn = std::make_unique<pqxx::connection>(m_connectionString);

        // Set schema search path
        pqxx::work txn(*conn);
        txn.exec(std::format("SET search_path TO {}", m_config.schema));
        txn.commit();

        return conn;
    }
    catch (const std::exception& e)
    {
        return nullptr;
    }
}

pqxx::result Database::Execute(const std::string& query)
{
    auto conn = GetConnection();
    if (!conn)
    {
        throw std::runtime_error("Failed to get database connection");
    }

    pqxx::work txn(*conn);
    pqxx::result result = txn.exec(query);
    txn.commit();

    return result;
}

Database& Database::Instance()
{
    static Database instance;
    return instance;
}

} // namespace database
