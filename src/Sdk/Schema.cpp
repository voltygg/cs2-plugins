#include "Schema.hpp"

#include "../Utils/Log.hpp"
#include "GameInterfaces.hpp"

#include <ISmmPlugin.h>

#include <map>
#include <schemasystem/schemasystem.h>
#include <string>

using namespace AdminSystem::Utils;

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

namespace AdminSystem::Sdk
{

// Cache: className -> (fieldName -> offset)
static std::map<std::string, std::map<std::string, int>> sOffsetCache;

bool InitSchemaSystem()
{
    if (!GameInterfaces::Instance().SchemaSystem)
    {
        Log::Warn("ISchemaSystem not available.");
        return false;
    }

    Log::Info("Schema system initialized.");
    return true;
}

int GetSchemaOffset(const char* className, const char* fieldName)
{
    auto* schemaSystem = GameInterfaces::Instance().SchemaSystem;
    if (!schemaSystem)
        return -1;

    // Check cache first
    auto classIt = sOffsetCache.find(className);
    if (classIt != sOffsetCache.end())
    {
        auto fieldIt = classIt->second.find(fieldName);
        if (fieldIt != classIt->second.end())
            return fieldIt->second;
    }

    // Look up in schema system
#ifdef _WIN32
    const char* moduleName = "server.dll";
#else
    const char* moduleName = "libserver.so";
#endif

    CSchemaSystemTypeScope* pTypeScope = schemaSystem->FindTypeScopeForModule(moduleName);
    if (!pTypeScope)
    {
        Log::Error("Schema: Failed to find type scope for {}.", moduleName);
        return -1;
    }

    SchemaMetaInfoHandle_t<CSchemaClassInfo> hClassInfo = pTypeScope->FindDeclaredClass(className);
    CSchemaClassInfo* pClassInfo = hClassInfo.Get();
    if (!pClassInfo)
    {
        Log::Error("Schema: Class '{}' not found.", className);
        return -1;
    }

    // Search fields
    for (int i = 0; i < pClassInfo->m_nFieldCount; ++i)
    {
        SchemaClassFieldData_t& field = pClassInfo->m_pFields[i];
        if (strcmp(field.m_pszName, fieldName) == 0)
        {
            int offset = field.m_nSingleInheritanceOffset;

            // Cache it
            sOffsetCache[className][fieldName] = offset;

            Log::Info("Schema: {}::{} = 0x{:X} ({})", className, fieldName, offset, offset);
            return offset;
        }
    }

    Log::Warn("Schema: Field '{}' not found in '{}'.", fieldName, className);
    return -1;
}

}  // namespace AdminSystem::Sdk
