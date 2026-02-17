#pragma once

#include <cstdint>

namespace AdminSystem::Sdk
{

/**
 * Call a virtual function by vtable index on a given object.
 *
 * Reads the vtable pointer from @p thisPtr, indexes into it, and invokes the
 * function with the forwarded arguments. Matches the CALL_VIRTUAL pattern used
 * throughout CS2 Metamod plugin references.
 *
 * @tparam Ret   Return type of the virtual function.
 * @tparam Args  Parameter types forwarded to the virtual function.
 * @param index   Zero-based vtable slot index.
 * @param thisPtr Pointer to the C++ object whose vtable is read.
 * @param args    Arguments forwarded to the virtual function.
 * @return The value returned by the virtual function.
 *
 * @note Vtable indices are platform-specific; load them from GameData at runtime.
 * @see GameData::GetOffset
 */
template <typename Ret, typename... Args>
constexpr Ret CallVirtual(int index, void* thisPtr, Args... args) noexcept
{
    auto vtable = *reinterpret_cast<void***>(thisPtr);
    auto fn = reinterpret_cast<Ret (*)(void*, Args...)>(vtable[index]);
    return fn(thisPtr, args...);
}

}  // namespace AdminSystem::Sdk
