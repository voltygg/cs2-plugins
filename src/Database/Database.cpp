#include "Database.hpp"

#include <format>
#include <stdexcept>

namespace AdminSystem::Database
{

std::string DatabaseConfig::GetConnectionString() const
{
    return std::format("host={} port={} dbname={} user={} password={} sslmode={}", Host, Port, DatabaseName, Username,
                       Password, SslMode);
}

bool Database::Initialize(const DatabaseConfig& config)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_initialized)
    {
        return true;
    }

    try
    {
        _config = config;
        _connectionString = config.GetConnectionString();

        // Test connection
        auto conn = std::make_unique<pqxx::connection>(_connectionString);
        if (!conn->is_open())
        {
            return false;
        }

        // Set schema search path
        pqxx::work txn(*conn);
        txn.exec(std::format("SET search_path TO {}", config.Schema));
        txn.commit();

        _initialized = true;
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
    std::lock_guard<std::mutex> lock(_mutex);
    _initialized = false;
}

bool Database::IsConnected() const
{
    return _initialized;
}

std::unique_ptr<pqxx::connection> Database::GetConnection()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_initialized)
    {
        return nullptr;
    }

    try
    {
        auto conn = std::make_unique<pqxx::connection>(_connectionString);

        // Set schema search path
        pqxx::work txn(*conn);
        txn.exec(std::format("SET search_path TO {}", _config.Schema));
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

}  // namespace AdminSystem::Database
