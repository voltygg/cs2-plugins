#include "TargetResolver.hpp"

#include "../Admin/AdminManager.hpp"
#include "../Core/Managers.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>

using CS2Kit::Core::Engine;

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

    auto& mgr = Engine().Players;
    auto& admins = App().Admins;

    auto add = [&](Player* p) {
        if (!p)
            return;
        ResolvedTarget r;
        r.Player = p;
        r.Allowed = caller ? admins.CanTarget(caller->GetSteamID(), p->GetSteamID()) : true;
        out.push_back(r);
    };

    const auto info = StringUtils::ParseTarget(token);
    switch (info.Type)
    {
    case StringUtils::TargetType::All:
        for (auto* p : mgr.GetAllPlayers())
            add(p);
        break;
    case StringUtils::TargetType::Me:
        add(caller);
        break;
    case StringUtils::TargetType::Index:
        add(mgr.GetPlayerBySlot(std::stoi(info.Value)));
        break;
    case StringUtils::TargetType::SteamId:
        add(mgr.GetPlayerBySteamId(std::stoll(info.Value)));
        break;
    case StringUtils::TargetType::Name:
    {
        const auto needle = StringUtils::ToLower(info.Value);
        for (auto* p : mgr.GetAllPlayers())
            if (p && StringUtils::ToLower(p->GetName()).find(needle) != std::string::npos)
                add(p);
        break;
    }
    }
    return out;
}

}  // namespace AdminSystem::Commands
