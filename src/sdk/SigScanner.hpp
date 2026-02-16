#pragma once

#include <cstdint>
#include <string>

namespace AdminSystem::Sdk {

void* FindPattern(const char* moduleName, const std::string& pattern);
uintptr_t ResolveRelativeAddress(uintptr_t addr, int ripOffset, int ripSize);

} // namespace AdminSystem::Sdk
