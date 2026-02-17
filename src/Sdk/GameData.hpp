#pragma once

#include "../Core/Singleton.hpp"

#include <string>
#include <unordered_map>

namespace AdminSystem::Sdk
{

/**
 * Centralized gamedata manager for platform-specific signatures and offsets.
 *
 * Loads byte-pattern signatures and named integer offsets from `signatures.jsonc`,
 * then resolves signatures at runtime via SigScanner. This replaces duplicated
 * gamedata parsing that was scattered across subsystems.
 *
 * @par File Format
 * The JSON file contains two sections:
 * - **signatures**: Named byte patterns with platform-specific variants and offsets.
 *   Used by FindSignature() and ResolveSignature().
 * - **offsets**: Named integer values (e.g. vtable indices, struct offsets).
 *   Used by GetOffset().
 *
 * @note The file path is resolved via PluginContext::ResolvePath().
 *
 * @see SigScanner For the byte pattern matching implementation.
 * @see CallVirtual Uses offsets loaded here for vtable calls.
 */
class GameData : public Core::Singleton<GameData>
{
public:
    explicit GameData(Token) {}

    /**
     * Load signatures and offsets from a gamedata JSON file.
     * @param path Relative or absolute path to the JSON file
     *             (e.g. "addons/admin-system/gamedata/signatures.jsonc").
     * @return True if the file was loaded and parsed successfully.
     */
    bool Load(const std::string& path);

    /**
     * Get a named offset value.
     * @param name Offset name (e.g. "GameEntitySystem", "CommitSuicide").
     * @return The integer offset, or -1 if not found.
     */
    int GetOffset(const std::string& name) const;

    /**
     * Scan for a named byte pattern and return the matched address.
     * @param name Signature name as defined in the gamedata file.
     * @return Pointer to the matched address in memory, or nullptr on failure.
     */
    void* FindSignature(const std::string& name) const;

    /**
     * Scan for a named pattern, then resolve a RIP-relative address.
     *
     * First finds the pattern match, then reads a 32-bit relative offset
     * at the entry's configured offset position and resolves it to an
     * absolute address (RIP-relative addressing).
     *
     * @param name Signature name as defined in the gamedata file.
     * @return Resolved absolute pointer, or nullptr on failure.
     */
    void* ResolveSignature(const std::string& name) const;

private:
    /** Internal representation of a byte-pattern signature entry. */
    struct SignatureEntry
    {
        std::string Library;  ///< Module name to scan (e.g. "server", "engine2").
        std::string Pattern;  ///< Byte pattern string (e.g. "48 8B 05 ?? ?? ?? ??").
        int Offset = 0;       ///< Offset within the matched pattern for RIP-relative resolution.
    };

    std::unordered_map<std::string, int> _offsets;                ///< Named offsets (e.g. vtable indices).
    std::unordered_map<std::string, SignatureEntry> _signatures;  ///< Named byte-pattern signatures.
};

}  // namespace AdminSystem::Sdk
