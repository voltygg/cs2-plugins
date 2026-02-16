#include "GameData.hpp"
#include "SigScanner.hpp"

#include <nlohmann/json.hpp>
#include <ISmmPlugin.h>

#include <filesystem>
#include <fstream>

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

namespace AdminSystem::Sdk {

bool GameData::Load(const std::string& path)
{
    try
    {
        auto fullPath = std::filesystem::path(g_SMAPI->GetBaseDir()) / path;
        std::ifstream file(fullPath);
        if (!file.is_open())
        {
            META_CONPRINTF("[AdminSystem] Warning: GameData file not found: %s\n", path.c_str());
            return false;
        }

        auto json = nlohmann::json::parse(file);

#ifdef _WIN32
        constexpr const char* platform = "windows";
#else
        constexpr const char* platform = "linux";
#endif

        // Load offsets
        if (json.contains("offsets"))
        {
            for (auto& [name, entry] : json["offsets"].items())
            {
                if (entry.contains(platform))
                    _offsets[name] = entry[platform].get<int>();
            }
        }

        // Load signatures
        if (json.contains("signatures"))
        {
            for (auto& [name, entry] : json["signatures"].items())
            {
                if (!entry.contains(platform))
                    continue;

                auto& platEntry = entry[platform];
                SignatureEntry sig;
                sig.Library = entry.value("library", "server");
                sig.Pattern = platEntry.value("pattern", "");
                sig.Offset = platEntry.value("offset", 0);
                _signatures[name] = std::move(sig);
            }
        }

        META_CONPRINTF("[AdminSystem] GameData loaded: %zu offsets, %zu signatures.\n",
                       _offsets.size(), _signatures.size());
        return true;
    }
    catch (const std::exception& e)
    {
        META_CONPRINTF("[AdminSystem] Warning: Failed to parse GameData: %s\n", e.what());
        return false;
    }
}

int GameData::GetOffset(const std::string& name) const
{
    auto it = _offsets.find(name);
    return it != _offsets.end() ? it->second : -1;
}

void* GameData::FindSignature(const std::string& name) const
{
    auto it = _signatures.find(name);
    if (it == _signatures.end())
        return nullptr;

    auto& sig = it->second;
    return Sdk::FindPattern(sig.Library.c_str(), sig.Pattern);
}

void* GameData::ResolveSignature(const std::string& name) const
{
    auto it = _signatures.find(name);
    if (it == _signatures.end())
        return nullptr;

    auto& sig = it->second;
    void* match = Sdk::FindPattern(sig.Library.c_str(), sig.Pattern);
    if (!match)
        return nullptr;

    if (sig.Offset == 0)
        return match;

    auto addr = reinterpret_cast<uintptr_t>(match) + sig.Offset;
    addr = ResolveRelativeAddress(addr, 0, 4);
    return reinterpret_cast<void*>(addr);
}

} // namespace AdminSystem::Sdk
