#pragma once

#include "Database/Repositories/Admins.hpp"
#include "Database/Repositories/Audit.hpp"
#include "Database/Repositories/Players.hpp"
#include "Database/Repositories/Punishments.hpp"
#include "Database/Repositories/Reports.hpp"

namespace AdminSystem::Database
{

/** Every repository, built once over the one @ref VoltMod::Database and injected by reference.
 *  Managers take this instead of the connection, so no call site builds one of its own. */
struct Repositories
{
    explicit Repositories(VoltMod::Database& db)
        : Admins(db), Punishments(db), Activity(db), Reports(db), Servers(db), Players(db)
    {}

    AdminRepository Admins;
    PunishmentRepository Punishments;
    AdminActivityRepository Activity;
    ReportRepository Reports;
    ServerRepository Servers;
    PlayerRepository Players;
};

}  // namespace AdminSystem::Database
