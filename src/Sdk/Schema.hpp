#pragma once

#include "../Core/Singleton.hpp"

#include <cstdint>
#include <map>
#include <string>

namespace AdminSystem::Sdk
{

/**
 * Runtime schema field offset resolution via ISchemaSystem.
 * Caches resolved offsets for O(1) repeated access.
 */
class SchemaService : public Core::Singleton<SchemaService>
{
public:
    explicit SchemaService(Token) {}

    /** Initialize the schema system (requires GameInterfaces::SchemaSystem to be set). */
    bool Initialize();

    /**
     * Look up the byte offset of a field within a schema class (results are cached).
     * Returns -1 if the class or field is not found.
     */
    int GetOffset(const char* className, const char* fieldName);

private:
    std::map<std::string, std::map<std::string, int>> _offsetCache;
};

}  // namespace AdminSystem::Sdk
