#pragma once

#include "Repositories/Admins.hpp"
#include "Repositories/Audit.hpp"
#include "Repositories/Players.hpp"
#include "Repositories/Punishments.hpp"

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
