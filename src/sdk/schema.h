#pragma once

#include <cstdint>

class ISchemaSystem;

namespace sdk {

// Global schema system pointer — set by plugin Load() via GET_V_IFACE_ANY
extern ISchemaSystem* g_pSchemaSystem;

/**
 * @brief Validate that the schema system interface is available.
 * Must be called after g_pSchemaSystem is set in plugin Load().
 * @return true if g_pSchemaSystem is set
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
