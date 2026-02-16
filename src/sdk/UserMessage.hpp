#pragma once

#include <string>

namespace AdminSystem::Sdk {

constexpr int UmSayText2 = 118;

bool InitMessageSystem();
bool InitGameEventManager();
void SendCenterHtml(int slot, const std::string& html);
void SendChatMessage(int slot, const std::string& message);
void ClearCenterHtml(int slot);

} // namespace AdminSystem::Sdk
