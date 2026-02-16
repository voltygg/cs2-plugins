#pragma once

#include "../Core/Singleton.hpp"
#include <string>
#include <unordered_map>

namespace AdminSystem::Utils {

/**
 * Localization system. Loads JSON translation files from a directory (one per language),
 * and provides key-based string lookup for the active language.
 */
class Translations : public Core::Singleton<Translations> {
    friend class Core::Singleton<Translations>;

public:
    bool Load(const std::string& dirPath);
    void SetLanguage(const std::string& lang);
    const std::string& GetLanguage() const;
    std::string Get(const std::string& key) const;

private:
    Translations() = default;

    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> _translations;
    std::string _activeLang = "en";
};

} // namespace AdminSystem::Utils
