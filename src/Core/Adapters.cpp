#include "Adapters.hpp"

#include "../../vendor/cs2-kit/src/Sdk/Entity.hpp"
#include "../../vendor/cs2-kit/src/Sdk/UserMessage.hpp"
#include "../Players/Player.hpp"

#include <Color.h>
#include <format>
#include <tier0/dbg.h>

namespace AdminSystem::Core
{

// HL2SDKLogger

void HL2SDKLogger::Info(const std::string& message)
{
    ConColorMsg(Color(0, 255, 0, 255), "%s\n", std::format("[AdminSystem] {}", message).c_str());
}

void HL2SDKLogger::Warn(const std::string& message)
{
    ConColorMsg(Color(255, 255, 0, 255), "%s\n", std::format("[AdminSystem] WARN: {}", message).c_str());
}

void HL2SDKLogger::Error(const std::string& message)
{
    ConColorMsg(Color(255, 0, 0, 255), "%s\n", std::format("[AdminSystem] ERROR: {}", message).c_str());
}

// SdkMenuIO

uint64_t SdkMenuIO::GetPlayerButtons(int slot)
{
    return CS2Kit::Sdk::EntitySystem::Instance().GetPlayerButtons(slot);
}

void SdkMenuIO::SendCenterHtml(int slot, const std::string& html)
{
    CS2Kit::Sdk::MessageSystem::Instance().SendCenterHtml(slot, html);
}

void SdkMenuIO::ClearCenterHtml(int slot)
{
    CS2Kit::Sdk::MessageSystem::Instance().ClearCenterHtml(slot);
}

// PlayerCaller

int64_t PlayerCaller::GetSteamID() const
{
    return _player ? _player->GetSteamID() : 0;
}

std::string PlayerCaller::GetName() const
{
    return _player ? _player->GetName() : "";
}

bool PlayerCaller::IsServerConsole() const
{
    return false;
}

}  // namespace AdminSystem::Core
