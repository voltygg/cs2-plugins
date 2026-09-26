#include "ClientLanguage.hpp"

#include <VoltMod/Hooks/ClientConVars.hpp>
#include <array>
#include <utility>

using VoltMod::ClientConVarStatus;

namespace MainMenu
{

struct SteamLanguage
{
    std::string_view Steam;
    std::string_view Code;
};

static constexpr std::array SteamLanguages{
    SteamLanguage{.Steam = "english", .Code = "en"},
    SteamLanguage{.Steam = "russian", .Code = "ru"},
};

ClientLanguage::ClientLanguage(VoltMod::Runtime& runtime) : _rt(runtime)
{
    _connected = _rt.Players.FullyConnected += [this](VoltMod::Player& player) { OnFullyConnected(player); };
}

std::string_view ClientLanguage::CodeFor(std::string_view steamLanguage)
{
    for (const SteamLanguage& language : SteamLanguages)
    {
        if (language.Steam == steamLanguage)
        {
            return language.Code;
        }
    }
    return {};
}

void ClientLanguage::OnFullyConnected(VoltMod::Player& player)
{
    const bool picked = !_rt.Translations.PlayerLanguage(player.Slot()).empty();
    if (player.IsBot() || picked)
    {
        return;
    }

    // Pending queries are dropped when the slot changes hands, so the answer is this player's.
    _rt.Hooks.ClientConVars.Query(
        player.Slot(), "cl_language",
        [this](int slot, ClientConVarStatus status, std::string_view, std::string_view value) {
            if (status != ClientConVarStatus::Answered)
            {
                return;
            }
            // A pick made while the query was out wins.
            const std::string_view code = CodeFor(value);
            if (!code.empty() && _rt.Translations.PlayerLanguage(slot).empty())
            {
                _rt.Translations.SetPlayerLanguage(slot, code);
            }
        });
}

}  // namespace MainMenu
