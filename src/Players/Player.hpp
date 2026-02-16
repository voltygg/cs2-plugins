#pragma once

#include <cstdint>
#include <string>

namespace AdminSystem::Players {

class Player {
public:
    Player(int slot, int64_t steamId, const std::string& name, const std::string& ipAddress);

    int GetSlot() const { return _slot; }
    int64_t GetSteamID() const { return _steamId; }
    const std::string& GetName() const { return _name; }
    const std::string& GetIpAddress() const { return _ipAddress; }
    int64_t GetConnectTime() const { return _connectTime; }
    bool IsAdmin() const { return _isAdmin; }
    bool IsMuted() const { return _isMuted; }
    bool IsGagged() const { return _isGagged; }
    int64_t GetPlaytime() const;

    void SetName(const std::string& name) { _name = name; }
    void SetAdmin(bool isAdmin) { _isAdmin = isAdmin; }
    void SetMuted(bool isMuted) { _isMuted = isMuted; }
    void SetGagged(bool isGagged) { _isGagged = isGagged; }

private:
    int _slot;
    int64_t _steamId;
    std::string _name;
    std::string _ipAddress;
    int64_t _connectTime;
    bool _isAdmin = false;
    bool _isMuted = false;
    bool _isGagged = false;
};

} // namespace AdminSystem::Players
