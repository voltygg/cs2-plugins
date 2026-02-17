#pragma once

#include <cstdint>

namespace AdminSystem::Sdk
{

/** Initialize the schema system (requires GameInterfaces::SchemaSystem to be set). */
bool InitSchemaSystem();
/**
 * Look up the byte offset of a field within a schema class (results are cached).
 * Returns -1 if the class or field is not found.
 */
int GetSchemaOffset(const char* className, const char* fieldName);

}  // namespace AdminSystem::Sdk
