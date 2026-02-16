#pragma once

#include <cstdint>

namespace AdminSystem::Sdk {

bool InitSchemaSystem();
int GetSchemaOffset(const char* className, const char* fieldName);

} // namespace AdminSystem::Sdk
