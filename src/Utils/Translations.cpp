#include "Translations.hpp"

#include "Log.hpp"

#include <ISmmPlugin.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

namespace AdminSystem::Utils
{

bool Translations::Load(const std::string& dirPath)
{
    _translations.clear();
    namespace fs = std::filesystem;

    fs::path resolvedPath(dirPath);
    if (resolvedPath.is_relative() && g_SMAPI)
        resolvedPath = fs::path(g_SMAPI->GetBaseDir()) / dirPath;

    if (!fs::exists(resolvedPath) || !fs::is_directory(resolvedPath))
    {
        Log::Warn("Translations directory not found: {}", resolvedPath.string());
        return false;
    }

    int loaded = 0;
    for (const auto& entry : fs::directory_iterator(resolvedPath))
    {
        if (entry.path().extension() != ".json")
            continue;

        std::string langCode = entry.path().stem().string();
        try
        {
            std::ifstream file(entry.path());
            if (!file.is_open())
                continue;

            auto data = nlohmann::json::parse(file);
            auto& langMap = _translations[langCode];
            for (auto& [key, value] : data.items())
            {
                if (value.is_string())
                    langMap[key] = value.get<std::string>();
            }

            ++loaded;
            Log::Info("Loaded translations: {} ({} keys)", langCode, langMap.size());
        }
        catch (const std::exception& e)
        {
            Log::Warn("Failed to parse {}: {}", entry.path().string(), e.what());
        }
    }

    Log::Info("Loaded {} language(s).", loaded);
    return loaded > 0;
}

void Translations::SetLanguage(const std::string& lang)
{
    _activeLang = lang;
}

const std::string& Translations::GetLanguage() const
{
    return _activeLang;
}

std::string Translations::Get(const std::string& key) const
{
    auto langIt = _translations.find(_activeLang);
    if (langIt != _translations.end())
    {
        auto keyIt = langIt->second.find(key);
        if (keyIt != langIt->second.end())
            return keyIt->second;
    }

    if (_activeLang != "en")
    {
        langIt = _translations.find("en");
        if (langIt != _translations.end())
        {
            auto keyIt = langIt->second.find(key);
            if (keyIt != langIt->second.end())
                return keyIt->second;
        }
    }

    return key;
}

}  // namespace AdminSystem::Utils
