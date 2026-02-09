#include "translations.h"

#include <ISmmPlugin.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

namespace utils {

bool Translations::Load(const std::string& dir_path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_translations.clear();

    namespace fs = std::filesystem;

    // Resolve relative paths against Metamod's game base directory (game/csgo/)
    fs::path resolved_path(dir_path);
    if (resolved_path.is_relative() && g_SMAPI)
    {
        resolved_path = fs::path(g_SMAPI->GetBaseDir()) / dir_path;
    }

    if (!fs::exists(resolved_path) || !fs::is_directory(resolved_path))
    {
        META_CONPRINTF("[AdminSystem] Warning: Translations directory not found: %s\n", resolved_path.string().c_str());
        return false;
    }

    int loaded = 0;
    for (const auto& entry : fs::directory_iterator(resolved_path))
    {
        if (entry.path().extension() != ".json")
            continue;

        std::string lang_code = entry.path().stem().string();

        try
        {
            std::ifstream file(entry.path());
            if (!file.is_open())
                continue;

            nlohmann::json data = nlohmann::json::parse(file);
            auto& lang_map = m_translations[lang_code];

            for (auto& [key, value] : data.items())
            {
                if (value.is_string())
                    lang_map[key] = value.get<std::string>();
            }

            ++loaded;
            META_CONPRINTF("[AdminSystem] Loaded translations: %s (%zu keys)\n",
                           lang_code.c_str(), lang_map.size());
        }
        catch (const std::exception& e)
        {
            META_CONPRINTF("[AdminSystem] Warning: Failed to parse %s: %s\n",
                           entry.path().string().c_str(), e.what());
        }
    }

    META_CONPRINTF("[AdminSystem] Loaded %d language(s).\n", loaded);
    return loaded > 0;
}

void Translations::SetLanguage(const std::string& lang)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeLang = lang;
}

std::string Translations::Get(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Try active language first
    auto lang_it = m_translations.find(m_activeLang);
    if (lang_it != m_translations.end())
    {
        auto key_it = lang_it->second.find(key);
        if (key_it != lang_it->second.end())
            return key_it->second;
    }

    // Fallback to English
    if (m_activeLang != "en")
    {
        lang_it = m_translations.find("en");
        if (lang_it != m_translations.end())
        {
            auto key_it = lang_it->second.find(key);
            if (key_it != lang_it->second.end())
                return key_it->second;
        }
    }

    // Return the key itself as last resort
    return key;
}

Translations& Translations::Instance()
{
    static Translations instance;
    return instance;
}

} // namespace utils
