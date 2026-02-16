#pragma once

#include "../Core/Singleton.hpp"
#include <string>
#include <unordered_map>

namespace AdminSystem::Sdk {

/**
 * Centralized gamedata manager. Loads platform-specific byte-pattern signatures
 * and offsets from signatures.jsonc, then resolves them at runtime via signature scanning.
 */
class GameData : public Core::Singleton<GameData> {
    friend class Core::Singleton<GameData>;

public:
    /** Load signatures and offsets from a gamedata directory (expects signatures.jsonc). */
    bool Load(const std::string& path);

    /** Get a named offset value (e.g. "GameEntitySystem"). */
    int GetOffset(const std::string& name) const;

    /** Scan for a named byte pattern and return the matched address (nullptr on failure). */
    void* FindSignature(const std::string& name) const;

    /** Scan for a named pattern, then resolve a RIP-relative address at the entry's offset. */
    void* ResolveSignature(const std::string& name) const;

private:
    GameData() = default;

    struct SignatureEntry {
        std::string Library;
        std::string Pattern;
        int Offset = 0;
    };

    std::unordered_map<std::string, int> _offsets;
    std::unordered_map<std::string, SignatureEntry> _signatures;
};

} // namespace AdminSystem::Sdk
