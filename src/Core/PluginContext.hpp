#pragma once

#include "Singleton.hpp"

#include <filesystem>
#include <string>

namespace AdminSystem::Core
{

/**
 * Plugin-wide context populated once during Plugin::Load().
 *
 * Stores the server base directory and provides path resolution utilities.
 * Eliminates the need for scattered `extern ISmmAPI* g_SMAPI` declarations
 * that were previously used solely to call `g_SMAPI->GetBaseDir()`.
 *
 * @note Populated in Plugin::Load() before any subsystem initialization.
 */
struct PluginContext : Singleton<PluginContext>
{
    explicit PluginContext(Token) {}

    /** Absolute path to the CS2 server root (e.g. `C:/cs2-server/game/csgo`). */
    std::filesystem::path BaseDir;

    /**
     * Resolve a relative path against the server base directory.
     * @param relativePath Path to resolve (e.g. `addons/admin-system/configs/settings.json`).
     * @return Absolute path. If @p relativePath is already absolute, it is returned as-is.
     */
    std::filesystem::path ResolvePath(const std::string& relativePath) const
    {
        std::filesystem::path p(relativePath);
        return p.is_absolute() ? p : BaseDir / p;
    }
};

}  // namespace AdminSystem::Core
