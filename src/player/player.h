#pragma once

#include <cstdint>
#include <string>

namespace player {

/**
 * @brief Player class representing a connected player
 */
class Player
{
public:
    Player(int slot, int64_t steam_id, const std::string& name, const std::string& ip_address);

    // Getters
    int GetSlot() const { return m_slot; }
    int64_t GetSteamID() const { return m_steamID; }
    const std::string& GetName() const { return m_name; }
    const std::string& GetIPAddress() const { return m_ipAddress; }
    int64_t GetConnectTime() const { return m_connectTime; }
    bool IsAdmin() const { return m_isAdmin; }
    bool IsMuted() const { return m_isMuted; }
    bool IsGagged() const { return m_isGagged; }

    // Setters
    void SetName(const std::string& name) { m_name = name; }
    void SetAdmin(bool is_admin) { m_isAdmin = is_admin; }
    void SetMuted(bool is_muted) { m_isMuted = is_muted; }
    void SetGagged(bool is_gagged) { m_isGagged = is_gagged; }

    /**
     * @brief Get playtime in seconds
     * @return Playtime since connection
     */
    int64_t GetPlaytime() const;

private:
    int m_slot;                    // Player slot index (0-63)
    int64_t m_steamID;             // SteamID64
    std::string m_name;            // Player name
    std::string m_ipAddress;       // IP address
    int64_t m_connectTime;         // Connection timestamp
    bool m_isAdmin;                // Is player an admin
    bool m_isMuted;                // Is player voice muted
    bool m_isGagged;               // Is player chat gagged
};

} // namespace player
