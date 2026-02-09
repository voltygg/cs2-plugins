#pragma once

#include <cstdint>

class ISchemaSystem;

namespace sdk {

/**
 * @brief Initialize the schema system. Call during plugin Load().
 * Obtains ISchemaSystem interface and prepares for offset lookups.
 * @return true if initialized successfully
 */
bool InitSchemaSystem();

/**
 * @brief Resolve a schema field offset at runtime.
 *
 * Looks up the class in the "server" module type scope and finds the field
 * by name, returning its offset within the class. Results are cached after
 * first lookup.
 *
 * @param className Schema class name (e.g. "CCSPlayerController")
 * @param fieldName Schema field name (e.g. "m_hPlayerPawn")
 * @return Field offset from class base, or -1 if not found
 */
int GetSchemaOffset(const char* className, const char* fieldName);

} // namespace sdk
