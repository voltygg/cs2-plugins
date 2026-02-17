#pragma once

#include "../../vendor/cs2-kit/src/Commands/ICommandCaller.hpp"
#include "../../vendor/cs2-kit/src/Core/ILogger.hpp"
#include "../../vendor/cs2-kit/src/Core/IPathResolver.hpp"
#include "../../vendor/cs2-kit/src/Menu/IMenuIO.hpp"

#include <filesystem>
#include <string>

namespace AdminSystem::Players
{
class Player;
}

namespace AdminSystem::Core
{

/**
 * Logger adapter: routes CS2Kit log calls to HL2SDK's ConColorMsg with [AdminSystem] prefix.
 */
class HL2SDKLogger : public CS2Kit::Core::ILogger
{
public:
    void Info(const std::string& message) override;
    void Warn(const std::string& message) override;
    void Error(const std::string& message) override;
};

/**
 * Path resolver adapter: resolves paths against the CS2 server base directory.
 */
class PluginPathResolver : public CS2Kit::Core::IPathResolver
{
public:
    void SetBaseDir(const std::filesystem::path& baseDir) { _baseDir = baseDir; }

    std::filesystem::path ResolvePath(const std::string& relativePath) const override
    {
        std::filesystem::path p(relativePath);
        return p.is_absolute() ? p : _baseDir / p;
    }

private:
    std::filesystem::path _baseDir;
};

/**
 * Menu I/O adapter: bridges CS2Kit menu system to HL2SDK entity/message systems.
 */
class SdkMenuIO : public CS2Kit::Menu::IMenuIO
{
public:
    uint64_t GetPlayerButtons(int slot) override;
    void SendCenterHtml(int slot, const std::string& html) override;
    void ClearCenterHtml(int slot) override;
};

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
