#pragma once

#include "Entities/Ban.hpp"
#include "Entities/TextMute.hpp"
#include "Entities/VoiceMute.hpp"
#include "Repositories/AdminActivityRepository.hpp"
#include "Repositories/AdminRepository.hpp"
#include "Repositories/PlayerRepository.hpp"
#include "Repositories/PunishmentRepository.hpp"
#include "Repositories/ReportRepository.hpp"
#include "Repositories/ServerRepository.hpp"
#include "Repositories/WarningRepository.hpp"

#include <VoltMod/Database/Api.hpp>

namespace AdminSystem::Database
{

/** Every repository, built once over the one @ref VoltMod::Database and injected by reference.
 *  Managers take this instead of the connection, so no call site builds one of its own. */
struct Repositories
{
    explicit Repositories(VoltMod::Database& db)
        : Admins(db),
          AdminGroups(db),
          AdminServerGroups(db),
          Activity(db),
          Bans(db),
          VoiceMutes(db),
          TextMutes(db),
          Warnings(db),
          Reports(db),
          Servers(db),
          Players(db)
    {}

    AdminRepository Admins;
    AdminGroupRepository AdminGroups;
    AdminServerGroupRepository AdminServerGroups;
    AdminActivityRepository Activity;
    PunishmentRepository<Ban> Bans;
    PunishmentRepository<VoiceMute> VoiceMutes;
    PunishmentRepository<TextMute> TextMutes;
    WarningRepository Warnings;
    ReportRepository Reports;
    ServerRepository Servers;
    PlayerRepository Players;
};

}  // namespace AdminSystem::Database
