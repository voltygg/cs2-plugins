#pragma once

#include "../Core/Singleton.hpp"
#include <string>
#include <unordered_map>

namespace AdminSystem::Sdk {

class GameData : public Core::Singleton<GameData> {
    friend class Core::Singleton<GameData>;

public:
    bool Load(const std::string& path);

    int GetOffset(const std::string& name) const;

    void* FindSignature(const std::string& name) const;

    // Find signature + resolve RIP-relative at the entry's offset
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
