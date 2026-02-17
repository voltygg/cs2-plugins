#pragma once

#include <cstdint>

class CEntityInstance;

namespace AdminSystem::Sdk
{

/**
 * Transient engine-side player wrapper around CCSPlayerController.
 *
 * Provides typed access to entity fields via the schema system, engine
 * operations (kick), and vtable operations (slay, change team, respawn).
 *
 * Complements the plugin-side Player class (Players/Player.hpp) which
 * tracks persistent plugin data (SteamID, admin flags, etc.).
 *
 * @note Create on the stack when you need engine access; do not store long-term.
 *       The underlying entity pointer may become invalid across ticks.
 *
 * @par Usage
 * @code
 * PlayerController ctrl(slot);
 * if (ctrl.IsValid())
 * {
 *     Log::Info("HP={}, Team={}", ctrl.GetHealth(), ctrl.GetTeam());
 *     ctrl.Kick("Cheating");
 * }
 * @endcode
 *
 * @see EntitySystem For the underlying entity resolution.
 * @see SchemaService For runtime field offset lookups.
 */
class PlayerController
{
public:
    /**
     * Construct a controller for the given player slot.
     * @param slot Player slot index (0-63).
     */
    explicit PlayerController(int slot);

    /**
     * Check whether the controller entity was resolved successfully.
     * @return True if the underlying CCSPlayerController entity is valid.
     */
    bool IsValid() const;

    /**
     * Get the raw CCSPlayerController entity pointer.
     * @return Entity pointer, or nullptr if invalid.
     */
    CEntityInstance* GetEntity() const;

    /**
     * Resolve the player's pawn entity (CCSPlayerPawn) from the controller.
     *
     * Reads `CCSPlayerController::m_hPlayerPawn` via the schema system
     * and resolves the entity handle through EntitySystem.
     *
     * @return Pawn entity pointer, or nullptr if the controller or pawn is invalid.
     */
    CEntityInstance* GetPawn() const;

    /** Get the player slot index this controller was created for. */
    int GetSlot() const { return _slot; }

    /// @name Engine Operations
    /// @{

    /**
     * Kick the player from the server.
     * @param reason Disconnect reason string shown to the player.
     *
     * @note Uses IVEngineServer2::DisconnectClient with NETWORK_DISCONNECT_KICKED.
     */
    void Kick(const char* reason) const;

    /// @}

    /// @name Generic Schema Field Access
    /// @{

    /**
     * Read a typed field from the controller entity by schema class and field name.
     * @tparam T The field type to read (e.g. int, uint32_t, float).
     * @param className  Schema class name (e.g. "CCSPlayerController").
     * @param fieldName  Schema field name (e.g. "m_iPing").
     * @return The field value, or T{} if the controller is invalid or the field is not found.
     */
    template <typename T>
    T GetField(const char* className, const char* fieldName) const;

    /**
     * Read a typed field from the pawn entity by schema class and field name.
     * @tparam T The field type to read.
     * @param className  Schema class name (e.g. "CBaseEntity", "CCSPlayerPawn").
     * @param fieldName  Schema field name (e.g. "m_iHealth").
     * @return The field value, or T{} if the pawn is invalid or the field is not found.
     */
    template <typename T>
    T GetPawnField(const char* className, const char* fieldName) const;

    /// @}

    /// @name Convenience Properties
    /// Properties read from the pawn entity via the schema system.
    /// @{

    /** Get the player's current health (CBaseEntity::m_iHealth). */
    int GetHealth() const;

    /** Get the player's team number (CBaseEntity::m_iTeamNum). */
    int GetTeam() const;

    /**
     * Get the player's life state (CBaseEntity::m_lifeState).
     * @return 0 = alive, 1 = dying, 2 = dead.
     */
    int GetLifeState() const;

    /** Check whether the player is alive (has a valid pawn with lifeState == 0). */
    bool IsAlive() const;

    /** Get the player's current button bitmask (delegates to EntitySystem). */
    uint64_t GetButtons() const;

    /** Get the player's armor value (CCSPlayerPawn::m_ArmorValue). */
    int GetArmor() const;

    /// @}

    /// @name Vtable Operations
    /// Operations performed via vtable calls using offsets loaded from GameData.
    /// @{

    /**
     * Kill the player by calling CommitSuicide on their pawn.
     * @note Requires the "CommitSuicide" vtable offset in gamedata.
     */
    void Slay() const;

    /**
     * Change the player's team.
     * @param team Target team number (1=Spectator, 2=T, 3=CT).
     * @note Requires the "ChangeTeam" vtable offset in gamedata.
     */
    void ChangeTeam(int team) const;

    /**
     * Respawn the player.
     * @note Requires the "Respawn" vtable offset in gamedata.
     */
    void Respawn() const;

    /// @}

private:
    int _slot;                               ///< Player slot index (0-63).
    CEntityInstance* _controller = nullptr;  ///< Cached CCSPlayerController pointer.
};

}  // namespace AdminSystem::Sdk
