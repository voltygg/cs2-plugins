#pragma once

#include <cstdint>
#include <string>

class CGameEntitySystem;
class CEntityInstance;

namespace AdminSystem::Sdk
{

/** @name Player button flags -- InputBitMask_t constants from the Source 2 engine. */
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

constexpr int MaxPlayers = 64;

/** Initialize the entity system by resolving CGameEntitySystem from GameResourceService. */
bool InitEntitySystem();
/** Get the global CGameEntitySystem pointer (nullptr if not initialized). */
CGameEntitySystem* GetEntitySystem();
/** Get the CCSPlayerController entity for a player slot (nullptr if invalid). */
CEntityInstance* GetPlayerController(int slot);
/** Read the current button bitmask for a player (traverses pawn -> movement services -> buttons). */
uint64_t GetPlayerButtons(int slot);
/** Check whether a player slot index is in the valid range [0, MaxPlayers). */
bool IsPlayerSlotValid(int slot);

}  // namespace AdminSystem::Sdk
