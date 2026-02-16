#include "schema.h"

#include <ISmmPlugin.h>
#include <schemasystem/schemasystem.h>

#include <map>
#include <string>

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

namespace sdk {

ISchemaSystem* g_pSchemaSystem = nullptr;

// Cache: className -> (fieldName -> offset)
static std::map<std::string, std::map<std::string, int>> s_offsetCache;

bool InitSchemaSystem()
{
    if (!g_pSchemaSystem)
    {
        META_CONPRINTF("[AdminSystem] Warning: ISchemaSystem not available.\n");
        return false;
    }

    META_CONPRINTF("[AdminSystem] Schema system initialized.\n");
    return true;
}

int GetSchemaOffset(const char* className, const char* fieldName)
{
    if (!g_pSchemaSystem)
        return -1;

    // Check cache first
    auto classIt = s_offsetCache.find(className);
    if (classIt != s_offsetCache.end())
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

    CSchemaSystemTypeScope* pTypeScope = g_pSchemaSystem->FindTypeScopeForModule(moduleName);
    if (!pTypeScope)
    {
        META_CONPRINTF("[AdminSystem] Schema: Failed to find type scope for %s.\n", moduleName);
        return -1;
    }

    SchemaMetaInfoHandle_t<CSchemaClassInfo> hClassInfo = pTypeScope->FindDeclaredClass(className);
    CSchemaClassInfo* pClassInfo = hClassInfo.Get();
    if (!pClassInfo)
    {
        META_CONPRINTF("[AdminSystem] Schema: Class '%s' not found.\n", className);
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
            s_offsetCache[className][fieldName] = offset;

            META_CONPRINTF("[AdminSystem] Schema: %s::%s = 0x%X (%d)\n",
                           className, fieldName, offset, offset);
            return offset;
        }
    }

    META_CONPRINTF("[AdminSystem] Schema: Field '%s' not found in '%s'.\n", fieldName, className);
    return -1;
}

} // namespace sdk
