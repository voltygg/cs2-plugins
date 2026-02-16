#pragma once

#include <cstdint>
#include <string>

namespace sdk {

/**
 * @brief Scan a loaded module for a byte pattern.
 *
 * @param moduleName Module name without extension (e.g. "server", "engine2").
 *                   On Windows appends ".dll", on Linux prepends "lib" and appends ".so".
 * @param pattern    Hex pattern with '?' wildcards (e.g. "48 83 EC ? 48 83 3D").
 * @return Address of first match, or nullptr if not found.
 */
void* FindPattern(const char* moduleName, const std::string& pattern);

/**
 * @brief Resolve a RIP-relative address.
 *
 * Common x64 pattern: the instruction at `addr` has a 32-bit relative offset
 * at byte position `ripOffset`, and the instruction is `ripSize` bytes total.
 * The target = addr + ripSize + *(int32_t*)(addr + ripOffset).
 *
 * @param addr      Base address (start of the instruction / pattern match).
 * @param ripOffset Byte offset within the instruction to the relative displacement.
 * @param ripSize   Total instruction size (displacement is relative to end of instruction).
 * @return Resolved absolute address.
 */
uintptr_t ResolveRelativeAddress(uintptr_t addr, int ripOffset, int ripSize);

} // namespace sdk
