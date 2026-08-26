#pragma once

#include "Config.hpp"

#include <VoltMod/Runtime.hpp>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>

namespace AdminSystem::Core
{

/**
 * Formats replies and admin broadcasts over `Runtime::Messages`. This service is
 * output-only so managers can depend on it without a cycle; PlayerChat owns input.
 */
class ChatService
{
public:
    ChatService(VoltMod::Runtime& runtime, const ConfigManager& config) : _rt(runtime), _config(config) {}

    /**
     * Send one chat line. Preserve existing color codes and prepend the default
     * color when none is present.
     */
    void Reply(int slot, std::string_view message);

    /**
     * Reply with a label line followed by the URL alone on its own line: CS2 chat doesn't wrap,
     * so any leading text (labels, wide nicknames) would push the URL off the panel's right edge.
     */
    void ReplyLink(int slot, std::string_view label, std::string_view url);

    /** Reply with a translated "no permission" line. */
    void NoPermission(int slot);

    /**
     * Broadcast an issued punishment, e.g. "[ADMIN] Bob banned Alice for cheating (1d)".
     * Skipped when `chat.broadcastPunishments` is false.
     */
    void BroadcastPunishment(std::string_view action, std::string_view adminName, std::string_view targetName,
                             std::string_view reason, int64_t durationSec);

    /**
     * Broadcast a translated server-wide notice with no admin or target name attached, e.g.
     * "Map changing to Dust II". Rendered in the server language, since one line goes to
     * everyone.
     */
    void BroadcastKey(const std::string& translationKey, const std::map<std::string, std::string>& tokens = {});

    /**
     * Broadcast a translated admin action. An empty target name represents a
     * self-targeted or server-wide action.
     */
    void BroadcastAction(const std::string& translationKey, std::string_view adminName, std::string_view targetName);

    /**
     * Token variant for multi-target actions, e.g. "[ADMIN] Bob swapped Alice and Carol".
     * The phrase at `translationKey` carries `{token}` placeholders matching @p nameTokens
     * keys; each name is substituted in with the same styling as the single-target layout.
     */
    void BroadcastAction(const std::string& translationKey, std::string_view adminName,
                         const std::map<std::string, std::string>& nameTokens);

private:
    VoltMod::Runtime& _rt;
    const ConfigManager& _config;

    /** Phrase at `translationKey`, or the key itself so a missing translation is obvious. */
    std::string BroadcastPhrase(const std::string& translationKey) const;
};

}  // namespace AdminSystem::Core
