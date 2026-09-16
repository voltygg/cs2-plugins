#pragma once

#include "Config/ConfigManager.hpp"

#include <VoltMod/Runtime.hpp>
#include <cstdint>
#include <map>
#include <optional>
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
    ChatService(VoltMod::Runtime& runtime, const Config::ConfigManager& config) : _rt(runtime), _config(config) {}

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
     * Broadcast an issued or lifted punishment, e.g. "[ADMIN] Bob banned Alice for cheating (1d)".
     * A nullopt duration prints none; 0 prints "permanent". Skipped when
     * `chat.broadcastPunishments` is false.
     */
    void BroadcastPunishment(std::string_view actionKey, std::string_view adminName, std::string_view targetName,
                             std::string_view reason, std::optional<int64_t> durationSec);

    /**
     * Broadcast a translated admin action. An empty admin name leaves the line unattributed, for
     * an event no one triggered; an empty target name represents a self-targeted or server-wide
     * action. Rendered in the server language, since one line goes to everyone.
     */
    void BroadcastAction(std::string_view translationKey, std::string_view adminName, std::string_view targetName);

    /**
     * Token variant for multi-target actions, e.g. "[ADMIN] Bob swapped Alice and Carol".
     * The phrase at `translationKey` carries `{token}` placeholders matching @p nameTokens
     * keys; each name is substituted in with the same styling as the single-target layout.
     */
    void BroadcastAction(std::string_view translationKey, std::string_view adminName,
                         const std::map<std::string, std::string>& nameTokens);

private:
    VoltMod::Runtime& _rt;
    const Config::ConfigManager& _config;

    /** Phrase at `translationKey`, or the key itself so a missing translation is obvious. */
    std::string BroadcastPhrase(std::string_view translationKey) const;
};

/** The name to attribute a broadcast to. Empty when the slot has emptied, which broadcasts
 *  unattributed. */
std::string ActorName(VoltMod::Runtime& runtime, int slot);

}  // namespace AdminSystem::Core
