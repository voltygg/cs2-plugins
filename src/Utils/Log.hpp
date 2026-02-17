#pragma once

#include <Color.h>
#include <format>
#include <string>
#include <tier0/dbg.h>

namespace AdminSystem::Utils::Log
{

namespace Detail
{

inline void Print(const Color& color, const std::string& message)
{
    ConColorMsg(color, "%s\n", message.c_str());
}

}  // namespace Detail

template <typename... Args>
void Info(std::format_string<Args...> fmt, Args&&... args)
{
    auto msg = std::format("[AdminSystem] {}", std::format(fmt, std::forward<Args>(args)...));
    Detail::Print(Color(0, 200, 200, 255), msg);
}

template <typename... Args>
void Warn(std::format_string<Args...> fmt, Args&&... args)
{
    auto msg = std::format("[AdminSystem] Warning: {}", std::format(fmt, std::forward<Args>(args)...));
    Detail::Print(Color(255, 200, 0, 255), msg);
}

template <typename... Args>
void Error(std::format_string<Args...> fmt, Args&&... args)
{
    auto msg = std::format("[AdminSystem] Error: {}", std::format(fmt, std::forward<Args>(args)...));
    Detail::Print(Color(255, 50, 50, 255), msg);
}

}  // namespace AdminSystem::Utils::Log
