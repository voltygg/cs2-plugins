#pragma once

#include "Singleton.hpp"

#include <filesystem>
#include <string>

namespace AdminSystem::Core
{

/**
 * Stores plugin-wide context (base directory) populated once during Plugin::Load().
 * Eliminates the need for scattered `extern ISmmAPI* g_SMAPI` declarations.
 */
struct PluginContext : Singleton<PluginContext>
{
    explicit PluginContext(Token) {}

    std::filesystem::path BaseDir;

    std::filesystem::path ResolvePath(const std::string& relativePath) const
    {
        std::filesystem::path p(relativePath);
        return p.is_absolute() ? p : BaseDir / p;
    }
};

}  // namespace AdminSystem::Core
