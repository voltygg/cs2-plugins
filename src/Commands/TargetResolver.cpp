#include "TargetResolver.hpp"

#include "../Admin/AdminManager.hpp"

#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>

namespace AdminSystem::Commands
{

using namespace CS2Kit::Players;
using namespace CS2Kit::Utils;
using AdminSystem::Admin::AdminManager;

std::vector<ResolvedTarget> Resolve(const std::string& token, Player* caller)
{
    std::vector<ResolvedTarget> out;
    if (token.empty())
        return out;

    auto& mgr = PlayerManager::Instance();
    auto& admins = AdminManager::Instance();

    auto check = [&](Player* p) {
        if (!p)
            return;
        ResolvedTarget r;
        r.Player = p;
        r.Allowed = caller ? admins.CanTarget(caller->GetSteamID(), p->GetSteamID()) : true;
        out.push_back(r);
    };

    if (token.size() > 1 && token[0] == '#' && StringUtils::IsNumeric(token.substr(1)))
    {
        int slot = std::stoi(token.substr(1));
        check(mgr.GetPlayerBySlot(slot));
        return out;
    }

    auto needle = StringUtils::ToLower(token);
    for (auto* p : mgr.GetAllPlayers())
    {
        if (!p)
            continue;
        if (StringUtils::ToLower(p->GetName()).find(needle) != std::string::npos)
            check(p);
    }
    return out;
}

}  // namespace AdminSystem::Commands
