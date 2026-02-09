#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

namespace utils {

/**
 * @brief Simple translation system. Loads key-value pairs from JSON files.
 * Falls back to English if a key is missing in the active language.
 */
class Translations
{
public:
    /**
     * @brief Load translations from a directory containing language JSON files.
     * @param dir_path Path to the translations directory (e.g. "addons/admin-system/configs/translations")
     * @return true if at least one language was loaded
     */
    bool Load(const std::string& dir_path);

    /**
     * @brief Set the active language.
     * @param lang Language code (e.g. "en", "ru")
     */
    void SetLanguage(const std::string& lang);

    /**
     * @brief Get the active language code.
     */
    const std::string& GetLanguage() const { return m_activeLang; }

    /**
     * @brief Get a translated string by key. Falls back to English, then returns the key itself.
     */
    std::string Get(const std::string& key) const;

    /**
     * @brief Get singleton instance.
     */
    static Translations& Instance();

private:
    Translations() = default;
    ~Translations() = default;
    Translations(const Translations&) = delete;
    Translations& operator=(const Translations&) = delete;

    // lang_code -> { key -> value }
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_translations;
    std::string m_activeLang = "en";
    mutable std::mutex m_mutex;
};

} // namespace utils
