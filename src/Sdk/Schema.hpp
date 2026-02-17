#pragma once

#include "../Core/Singleton.hpp"

#include <cstdint>
#include <map>
#include <string>

namespace AdminSystem::Sdk
{

/**
 * Runtime schema field offset resolution via ISchemaSystem.
 *
 * Provides access to entity field offsets at runtime by querying the
 * engine's schema system. Results are cached in a nested map for O(1)
 * repeated access after the first lookup.
 *
 * The schema system is how Source 2 exposes its entity class layouts at
 * runtime, allowing plugins to read entity fields without hardcoded offsets.
 *
 * @note Requires GameInterfaces::SchemaSystem to be set before Initialize().
 *
 * @par Usage
 * @code
 * int offset = SchemaService::Instance().GetOffset("CBaseEntity", "m_iHealth");
 * if (offset >= 0)
 * {
 *     int health = *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(entity) + offset);
 * }
 * @endcode
 *
 * @see EntitySystem Uses SchemaService for resolving pawn/button chain offsets.
 * @see PlayerController Uses SchemaService for generic field reads.
 */
class SchemaService : public Core::Singleton<SchemaService>
{
public:
    explicit SchemaService(Token) {}

    /**
     * Initialize the schema system.
     *
     * Verifies that `GameInterfaces::SchemaSystem` is available.
     *
     * @return True if ISchemaSystem is available, false otherwise.
     */
    bool Initialize();

    /**
     * Look up the byte offset of a field within a schema class.
     *
     * On first call for a given class/field pair, queries `ISchemaSystem` via
     * `FindTypeScopeForModule("server.dll")` and caches the result.
     *
     * @param className Schema class name (e.g. "CBaseEntity").
     * @param fieldName Schema field name (e.g. "m_iHealth").
     * @return Byte offset from the entity base pointer, or -1 if not found.
     */
    int GetOffset(const char* className, const char* fieldName);

private:
    /// Nested cache: className -> { fieldName -> offset }.
    std::map<std::string, std::map<std::string, int>> _offsetCache;
};

}  // namespace AdminSystem::Sdk
