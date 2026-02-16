#pragma once

#include "../Core/Singleton.hpp"
#include "../Database/Entities/Ban.hpp"
#include "../Database/Entities/Mute.hpp"
#include "../Database/Entities/Gag.hpp"
#include "../Database/Entities/Warning.hpp"
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace AdminSystem::Punishments {

class PunishmentManager : public Core::Singleton<PunishmentManager> {
    friend class Core::Singleton<PunishmentManager>;

public:
    bool LoadActivePunishments();
    std::optional<Database::Ban> GetActiveBan(int64_t steamId);
    bool IsMuted(int64_t steamId);
    bool IsGagged(int64_t steamId);
    bool IssueBan(const Database::Ban& ban);
    bool IssueMute(const Database::Mute& mute);
    bool IssueGag(const Database::Gag& gag);
    bool IssueWarning(const Database::Warning& warning);
    bool RemoveBan(int64_t banId, int64_t removedBy, const std::string& reason);
    bool RemoveMute(int64_t muteId, int64_t removedBy, const std::string& reason);
    bool RemoveGag(int64_t gagId, int64_t removedBy, const std::string& reason);
    void ExpireOldPunishments();

private:
    PunishmentManager() = default;

    std::unordered_set<int64_t> _mutedPlayers;
    std::unordered_set<int64_t> _gaggedPlayers;
    std::unordered_map<int64_t, Database::Ban> _activeBans;
};

} // namespace AdminSystem::Punishments
