#pragma once

#include <cstdint>
#include <string>

// Forward declarations from HL2SDK
class CGameEntitySystem;
class CEntityInstance;

namespace sdk {

// Player button flags (InputBitMask_t from Source 2)
constexpr uint64_t IN_ATTACK    = 0x1;
constexpr uint64_t IN_JUMP      = 0x2;
constexpr uint64_t IN_DUCK      = 0x4;
constexpr uint64_t IN_FORWARD   = 0x8;
constexpr uint64_t IN_BACK      = 0x10;
constexpr uint64_t IN_USE       = 0x20;
constexpr uint64_t IN_TURNLEFT  = 0x80;
constexpr uint64_t IN_TURNRIGHT = 0x100;
constexpr uint64_t IN_MOVELEFT  = 0x200;
constexpr uint64_t IN_MOVERIGHT = 0x400;
constexpr uint64_t IN_ATTACK2   = 0x800;
constexpr uint64_t IN_RELOAD    = 0x2000;
constexpr uint64_t IN_SPEED     = 0x10000;
constexpr uint64_t IN_SCORE     = 0x200000000ULL;
constexpr uint64_t IN_ZOOM      = 0x400000000ULL;
constexpr uint64_t IN_LOOK_AT_WEAPON = 0x800000000ULL;

constexpr int MAXPLAYERS = 64;

/**
 * @brief Initialize entity access. Call during plugin Load().
 * Loads gamedata offsets and obtains entity system pointer.
 * @return true if initialized successfully
 */
bool InitEntitySystem();

/**
 * @brief Get the entity system pointer.
 */
CGameEntitySystem* GetEntitySystem();

/**
 * @brief Get the player controller entity for a given slot.
 * @param slot Player slot (0-63)
 * @return Entity instance pointer, or nullptr if not valid
 */
CEntityInstance* GetPlayerController(int slot);

/**
 * @brief Read button state for a player via the schema-resolved pointer chain:
 * Controller -> m_hPlayerPawn -> Pawn -> m_pMovementServices -> m_nButtons -> m_pButtonStates[0]
 * @param slot Player slot (0-63)
 * @return Current button bitmask, or 0 if unavailable
 */
uint64_t GetPlayerButtons(int slot);

/**
 * @brief Check if a specific player slot has a valid entity.
 */
bool IsPlayerSlotValid(int slot);

// Global entity system pointer
extern CGameEntitySystem* g_pEntitySystem;

} // namespace sdk
