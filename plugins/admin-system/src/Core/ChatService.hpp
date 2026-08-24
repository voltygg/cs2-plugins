#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>

namespace VoltMod
{
class Runtime;
}

namespace AdminSystem::Core
{

class ConfigManager;

/**
 * Renders admin-system chat semantics on top of `VoltMod::Runtime::Messages`: styled per-command
 * replies and admin action/punishment broadcasts.
 *
 * Output only, and deliberately so - it reads nothing but the runtime and config, which is what
 * lets the managers that broadcast (freezes, punishments, cheat checks) depend on it without a
 * cycle. Reading a player's chat back is @ref PlayerChat's job.
 */
class ChatService
{
public:
    ChatService(VoltMod::Runtime& runtime, const ConfigManager& config) : _rt(runtime), _config(config) {}

    /**
     * Send a single-line reply to one player. Color codes inside `message` are honored;
     * `Default` color is prepended automatically when missing.
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
     * Broadcast a non-punishment admin action, e.g. "[ADMIN] Bob slapped Alice".
     * The verb is read from `translationKey`; both `adminName` and `targetName` are interpolated.
     * Pass an empty `targetName` for self-targeted or server-wide actions.
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
