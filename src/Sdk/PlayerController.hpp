#pragma once

#include <cstdint>

class CEntityInstance;

namespace AdminSystem::Sdk
{

/**
 * Transient engine-side player wrapper around CCSPlayerController.
 * Provides typed access to entity fields via the schema system, engine
 * operations (kick), and vtable operations (slay, change team, respawn).
 *
 * Complements the plugin-side Player class (Players/Player.hpp) which
 * tracks persistent plugin data (SteamID, admin flags, etc.).
 *
 * Usage: Create on the stack when you need engine access, don't store long-term.
 *   PlayerController ctrl(slot);
 *   if (ctrl.IsValid()) ctrl.Kick("Cheating");
 */
class PlayerController
{
public:
    explicit PlayerController(int slot);

    bool IsValid() const;
    CEntityInstance* GetEntity() const;
    CEntityInstance* GetPawn() const;

    int GetSlot() const { return _slot; }

    // Engine operations (IVEngineServer2)
    void Kick(const char* reason) const;

    // Generic schema field access
    template <typename T>
    T GetField(const char* className, const char* fieldName) const;

    template <typename T>
    T GetPawnField(const char* className, const char* fieldName) const;

    // Convenience properties (read from pawn via schema)
    int GetHealth() const;
    int GetTeam() const;
    int GetLifeState() const;
    bool IsAlive() const;
    uint64_t GetButtons() const;
    int GetArmor() const;

    // Vtable operations (offsets from GameData)
    void Slay() const;
    void ChangeTeam(int team) const;
    void Respawn() const;

private:
    int _slot;
    CEntityInstance* _controller = nullptr;
};

}  // namespace AdminSystem::Sdk
