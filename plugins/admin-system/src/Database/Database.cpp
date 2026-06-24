#include "Database.hpp"

#include <CS2Kit/Utils/Log.hpp>
#include <format>
#include <stdexcept>

namespace AdminSystem::Database
{

std::string DatabaseConfig::GetConnectionString() const
{
    return std::format("host={} port={} dbname={} user={} password={} sslmode={}", host, port, database, username,
                       password, sslMode);
}

bool Database::Initialize(const DatabaseConfig& config)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _connectionString = config.GetConnectionString();

    try
    {
        EnsureOpen();
        return true;
    }
    catch (const std::exception&)
    {
        // Don't log the exception text: a failed pqxx::connection ctor can echo the full DSN
        // (password included). Report a generic, secret-free message instead.
        CS2Kit::Utils::Log::Error("Database connection failed - check host/port/credentials in settings.jsonc.");
        return false;
    }
}

void Database::CloseConnection()
{
    std::lock_guard<std::mutex> lock(_mutex);
    Reset();
}

bool Database::IsConnected() const
{
    return _connection && _connection->is_open();
}

pqxx::result Database::Execute(const std::string& query)
{
    std::lock_guard<std::mutex> lock(_mutex);
    EnsureOpen();

    pqxx::work txn(*_connection);
    pqxx::result result = txn.exec(query);
    txn.commit();
    return result;
}

void Database::EnsureOpen()
{
    if (_connection && _connection->is_open())
        return;

    // A reopened socket has no server-side prepared statements; forget the cache so each name is
    // re-prepared on first use against the new connection.
    _prepared.clear();
    _connection = std::make_unique<pqxx::connection>(_connectionString);
    if (!_connection->is_open())
    {
        _connection.reset();
        throw std::runtime_error("Failed to open database connection");
    }
}

void Database::Reset()
{
    _connection.reset();
    _prepared.clear();
}

}  // namespace AdminSystem::Database
