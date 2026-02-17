#pragma once

#include <cstdint>

namespace AdminSystem::Sdk
{

/**
 * Call a virtual function by vtable index on a given object.
 * Matches the CALL_VIRTUAL pattern from CS2 plugin references.
 */
template <typename Ret, typename... Args>
constexpr Ret CallVirtual(int index, void* thisPtr, Args... args) noexcept
{
    auto vtable = *reinterpret_cast<void***>(thisPtr);
    auto fn = reinterpret_cast<Ret (*)(void*, Args...)>(vtable[index]);
    return fn(thisPtr, args...);
}

}  // namespace AdminSystem::Sdk
