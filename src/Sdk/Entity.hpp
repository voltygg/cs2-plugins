#pragma once

#include "../Core/Singleton.hpp"

#include <cstdint>
#include <string>

class CGameEntitySystem;
class CEntityInstance;
class CEntityIdentity;

namespace AdminSystem::Sdk
{

/**
 * @defgroup ButtonFlags Player Button Flags
 * @brief InputBitMask_t constants from the Source 2 engine.
 *
 * These correspond to player input actions and can be tested against
 * the bitmask returned by EntitySystem::GetPlayerButtons().
 * @{
 */
constexpr uint64_t IN_ATTACK = 0x1;
constexpr uint64_t IN_JUMP = 0x2;
constexpr uint64_t IN_DUCK = 0x4;
constexpr uint64_t IN_FORWARD = 0x8;
constexpr uint64_t IN_BACK = 0x10;
constexpr uint64_t IN_USE = 0x20;
constexpr uint64_t IN_TURNLEFT = 0x80;
constexpr uint64_t IN_TURNRIGHT = 0x100;
constexpr uint64_t IN_MOVELEFT = 0x200;
constexpr uint64_t IN_MOVERIGHT = 0x400;
constexpr uint64_t IN_ATTACK2 = 0x800;
constexpr uint64_t IN_RELOAD = 0x2000;
constexpr uint64_t IN_SPEED = 0x10000;
constexpr uint64_t IN_SCORE = 0x200000000ULL;
constexpr uint64_t IN_ZOOM = 0x400000000ULL;
constexpr uint64_t IN_LOOK_AT_WEAPON = 0x800000000ULL;
/** @} */

/** Maximum number of player slots in a CS2 server. */
constexpr int MaxPlayers = 64;

/**
 * Entity system access layer for the Source 2 engine.
 *
 * Resolves `CGameEntitySystem` from `IGameResourceService`, provides player
 * controller lookup by slot, entity handle resolution, and button state reading.
 *
 * The button state is read by traversing the entity chain:
 * Controller -> m_hPlayerPawn -> Pawn -> m_pMovementServices -> m_nButtons -> m_pButtonStates
 *
 * @note Schema offsets for the pawn/movement/button chain are resolved lazily
 *       on first use via SchemaService.
 *
 * @see SchemaService For how field offsets are resolved.
 * @see PlayerController For a higher-level player wrapper built on top of this.
 */
class EntitySystem : public Core::Singleton<EntitySystem>
{
public:
    explicit EntitySystem(Token) {}

    /**
     * Initialize the entity system by resolving CGameEntitySystem.
     *
     * Reads the entity system pointer from `IGameResourceService` at the
     * "GameEntitySystem" offset loaded from GameData.
     *
     * @return True if CGameEntitySystem was resolved, false otherwise.
     */
    bool Initialize();

    /**
     * Get the global CGameEntitySystem pointer.
     * @return The entity system pointer, or nullptr if not initialized.
     */
    CGameEntitySystem* GetEntitySystem();

    /**
     * Get the CCSPlayerController entity for a player slot.
     * @param slot Player slot index (0-63).
     * @return Entity instance pointer, or nullptr if the slot is invalid or empty.
     */
    CEntityInstance* GetPlayerController(int slot);

    /**
     * Resolve a CHandle (entity handle) to a CEntityInstance pointer.
     *
     * Entity handles encode an entity index and serial number. This method
     * extracts the index, looks up the CEntityIdentity in the chunked entity
     * list, and validates the serial number.
     *
     * @param handle Raw entity handle value (e.g. from m_hPlayerPawn).
     * @return Resolved entity pointer, or nullptr if the handle is invalid.
     */
    CEntityInstance* ResolveEntityHandle(uint32_t handle);

    /**
     * Read the current button bitmask for a player.
     *
     * Traverses: Controller -> m_hPlayerPawn -> Pawn -> m_pMovementServices
     * -> m_nButtons -> m_pButtonStates[0].
     *
     * @param slot Player slot index (0-63).
     * @return Bitmask of currently pressed buttons (see @ref ButtonFlags),
     *         or 0 if any entity in the chain is invalid.
     */
    uint64_t GetPlayerButtons(int slot);

    /**
     * Check whether a player slot has a valid controller entity.
     * @param slot Player slot index (0-63).
     * @return True if a valid CCSPlayerController exists for this slot.
     */
    bool IsPlayerSlotValid(int slot);

private:
    /**
     * Lazily resolve schema offsets for the pawn/button chain.
     *
     * Resolves: m_hPlayerPawn, m_pMovementServices, m_nButtons, m_pButtonStates
     * via SchemaService. Results are cached for the lifetime of the singleton.
     */
    void ResolveSchemaOffsets();

    /**
     * Look up a CEntityIdentity by index in the chunked entity list.
     * @param pSys  The entity system to search.
     * @param index Entity index to look up.
     * @return Identity pointer, or nullptr if out of bounds.
     */
    CEntityIdentity* GetEntityIdentityByIndex(CGameEntitySystem* pSys, int index);

    int _offsetPlayerPawn = -1;           ///< Schema offset: CCSPlayerController::m_hPlayerPawn
    int _offsetMovementServices = -1;     ///< Schema offset: CCSPlayerPawn::m_pMovementServices
    int _offsetButtons = -1;              ///< Schema offset: CCSPlayer_MovementServices::m_nButtons
    int _offsetButtonStates = -1;         ///< Schema offset: CInButtonState::m_pButtonStates
    bool _schemaOffsetsResolved = false;  ///< Whether ResolveSchemaOffsets() has run.
};

}  // namespace AdminSystem::Sdk
