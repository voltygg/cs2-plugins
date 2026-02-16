#include "sigscanner.h"

#include <ISmmPlugin.h>

#include <sstream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <dlfcn.h>
#include <link.h>
#include <cstring>
#endif

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

namespace sdk {

struct PatternByte
{
    uint8_t value;
    bool wildcard;
};

static std::vector<PatternByte> ParsePattern(const std::string& pattern)
{
    std::vector<PatternByte> bytes;
    std::istringstream stream(pattern);
    std::string token;

    while (stream >> token)
    {
        if (token == "?" || token == "??")
        {
            bytes.push_back({0, true});
        }
        else
        {
            bytes.push_back({static_cast<uint8_t>(std::stoul(token, nullptr, 16)), false});
        }
    }
    return bytes;
}

static void* ScanMemory(const uint8_t* base, size_t size, const std::vector<PatternByte>& pattern)
{
    if (pattern.empty() || size < pattern.size())
        return nullptr;

    size_t scanEnd = size - pattern.size();
    for (size_t i = 0; i <= scanEnd; ++i)
    {
        bool found = true;
        for (size_t j = 0; j < pattern.size(); ++j)
        {
            if (!pattern[j].wildcard && base[i + j] != pattern[j].value)
            {
                found = false;
                break;
            }
        }
        if (found)
            return const_cast<uint8_t*>(base + i);
    }
    return nullptr;
}

#ifdef _WIN32

// On Windows, Metamod loads a small stub server.dll that intercepts the game's server.dll.
// GetModuleHandleA("server.dll") returns the stub, not the real game module.
// We enumerate all loaded modules and pick the largest one matching the filename.
static bool GetModuleInfo(const char* moduleName, uint8_t*& base, size_t& size)
{
    HANDLE hProcess = GetCurrentProcess();
    HMODULE hModules[1024];
    DWORD cbNeeded = 0;

    if (!EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded))
        return false;

    DWORD moduleCount = cbNeeded / sizeof(HMODULE);
    HMODULE bestModule = nullptr;
    DWORD bestSize = 0;

    for (DWORD i = 0; i < moduleCount; ++i)
    {
        char modPath[MAX_PATH];
        if (!GetModuleFileNameA(hModules[i], modPath, sizeof(modPath)))
            continue;

        // Extract filename from full path
        const char* fileName = strrchr(modPath, '\\');
        if (!fileName)
            fileName = strrchr(modPath, '/');
        fileName = fileName ? fileName + 1 : modPath;

        if (_stricmp(fileName, moduleName) != 0)
            continue;

        // Get module size — pick the largest one (real server vs Metamod stub)
        MODULEINFO modInfo{};
        if (GetModuleInformation(hProcess, hModules[i], &modInfo, sizeof(modInfo)))
        {
            if (modInfo.SizeOfImage > bestSize)
            {
                bestModule = hModules[i];
                bestSize = modInfo.SizeOfImage;
            }
        }
    }

    if (!bestModule)
        return false;

    MODULEINFO modInfo{};
    if (!GetModuleInformation(hProcess, bestModule, &modInfo, sizeof(modInfo)))
        return false;

    base = static_cast<uint8_t*>(modInfo.lpBaseOfDll);
    size = modInfo.SizeOfImage;
    return true;
}

#else

struct ModuleInfo
{
    const char* name;
    uint8_t* base;
    size_t size;
    bool found;
};

static int dl_iterate_callback(struct dl_phdr_info* info, size_t /*size*/, void* data)
{
    auto* mod = static_cast<ModuleInfo*>(data);
    if (info->dlpi_name && strstr(info->dlpi_name, mod->name))
    {
        mod->base = reinterpret_cast<uint8_t*>(info->dlpi_addr);
        // Sum up all LOAD segments to get total mapped size
        size_t maxAddr = 0;
        for (int i = 0; i < info->dlpi_phnum; ++i)
        {
            if (info->dlpi_phdr[i].p_type == PT_LOAD)
            {
                size_t segEnd = info->dlpi_phdr[i].p_vaddr + info->dlpi_phdr[i].p_memsz;
                if (segEnd > maxAddr)
                    maxAddr = segEnd;
            }
        }
        mod->size = maxAddr;
        mod->found = true;
        return 1; // Stop iteration
    }
    return 0;
}

static bool GetModuleInfo(const char* moduleName, uint8_t*& base, size_t& size)
{
    ModuleInfo mod{moduleName, nullptr, 0, false};
    dl_iterate_phdr(dl_iterate_callback, &mod);
    if (mod.found)
    {
        base = mod.base;
        size = mod.size;
    }
    return mod.found;
}

#endif

void* FindPattern(const char* moduleName, const std::string& pattern)
{
    // Build platform-specific module filename
    std::string fullName;
#ifdef _WIN32
    fullName = std::string(moduleName) + ".dll";
#else
    fullName = std::string("lib") + moduleName + ".so";
#endif

    uint8_t* base = nullptr;
    size_t size = 0;
    if (!GetModuleInfo(fullName.c_str(), base, size))
    {
        META_CONPRINTF("[AdminSystem] SigScanner: Module '%s' not found.\n", fullName.c_str());
        return nullptr;
    }

    META_CONPRINTF("[AdminSystem] SigScanner: Scanning '%s' (base=%p, size=0x%zX)...\n",
                   fullName.c_str(), static_cast<void*>(base), size);

    auto patternBytes = ParsePattern(pattern);
    void* result = ScanMemory(base, size, patternBytes);
    if (!result)
    {
        META_CONPRINTF("[AdminSystem] SigScanner: Pattern not found in '%s'.\n", fullName.c_str());
    }
    else
    {
        META_CONPRINTF("[AdminSystem] SigScanner: Found match at %p.\n", result);
    }
    return result;
}

uintptr_t ResolveRelativeAddress(uintptr_t addr, int ripOffset, int ripSize)
{
    int32_t relative = *reinterpret_cast<int32_t*>(addr + ripOffset);
    return addr + ripSize + relative;
}

} // namespace sdk
