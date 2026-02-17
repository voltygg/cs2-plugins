#pragma once

#include <CS2Kit/Commands/ICommandCaller.hpp>

#include <string>

namespace AdminSystem::Players
{
class Player;
}

namespace AdminSystem::Core
{

/**
 * Command caller adapter: wraps Players::Player* to implement ICommandCaller.
 */
class PlayerCaller : public CS2Kit::Commands::ICommandCaller
{
public:
    explicit PlayerCaller(Players::Player* player) : _player(player) {}

    int64_t GetSteamID() const override;
    std::string GetName() const override;
    bool IsServerConsole() const override;

    Players::Player* GetPlayer() const { return _player; }

private:
    Players::Player* _player;
};

}  // namespace AdminSystem::Core
