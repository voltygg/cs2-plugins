#pragma once

// This plugin's one forward declaration, in one place - the same arrangement
// <VoltMod/Engine/EngineTypes.hpp> makes for the framework. Everything else includes the
// header that defines what it names.

namespace Anticheat
{

/**
 * AntiCheatManager owns every detector below it by value, so AntiCheatManager.hpp includes
 * their headers; a detector header therefore cannot include AntiCheatManager.hpp back. The
 * detectors call four gates on it (DetectionsEnabled, ModuleEnabled, IsEligible, Report) plus
 * the cores they score against. Giving them those directly, instead of the whole manager,
 * would retire this line.
 */
class AntiCheatManager;

}  // namespace Anticheat
