#pragma once

#include "../Core/Singleton.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

namespace AdminSystem::Sdk
{

/**
 * Typed wrapper around ICvar for finding, reading, writing, and listening to ConVars.
 *
 * Provides a clean API over the HL2SDK ConVar system, supporting typed
 * read/write access and global change notification. Also exposes server
 * command execution via IVEngineServer2.
 *
 * @note ConVar names are looked up on every Get/Set call via ConVarRefAbstract.
 *       For hot-path access, consider caching the result.
 *
 * @par Usage
 * @code
 * auto maxRate = ConVarService::Instance().GetInt("sv_maxrate");
 * if (maxRate) Log::Info("sv_maxrate = {}", *maxRate);
 *
 * ConVarService::Instance().SetInt("sv_cheats", 1);
 * ConVarService::Instance().ExecuteServerCommand("changelevel de_dust2");
 * @endcode
 */
class ConVarService : public Core::Singleton<ConVarService>
{
public:
    explicit ConVarService(Token) {}

    /**
     * Initialize the ConVar service.
     * @return True if ICvar is available, false otherwise.
     */
    bool Initialize();

    /// @name Read Operations
    /// @{

    /**
     * Get a ConVar's value as an integer.
     * @param name ConVar name (e.g. "sv_cheats").
     * @return The integer value, or std::nullopt if the ConVar does not exist.
     */
    std::optional<int> GetInt(const char* name) const;

    /**
     * Get a ConVar's value as a float.
     * @param name ConVar name.
     * @return The float value, or std::nullopt if the ConVar does not exist.
     */
    std::optional<float> GetFloat(const char* name) const;

    /**
     * Get a ConVar's value as a string.
     * @param name ConVar name.
     * @return The string value, or std::nullopt if the ConVar does not exist.
     */
    std::optional<std::string> GetString(const char* name) const;

    /**
     * Get a ConVar's value as a boolean.
     * @param name ConVar name.
     * @return The boolean value, or std::nullopt if the ConVar does not exist.
     */
    std::optional<bool> GetBool(const char* name) const;

    /**
     * Check whether a ConVar exists and is registered.
     * @param name ConVar name.
     * @return True if the ConVar exists and has valid data.
     */
    bool Exists(const char* name) const;

    /// @}

    /// @name Write Operations
    /// @{

    /**
     * Set a ConVar's value as an integer.
     * @param name  ConVar name.
     * @param value New integer value.
     * @return True on success, false if the ConVar does not exist.
     */
    bool SetInt(const char* name, int value);

    /**
     * Set a ConVar's value as a float.
     * @param name  ConVar name.
     * @param value New float value.
     * @return True on success, false if the ConVar does not exist.
     */
    bool SetFloat(const char* name, float value);

    /**
     * Set a ConVar's value as a string.
     * @param name  ConVar name.
     * @param value New string value.
     * @return True on success, false if the ConVar does not exist.
     */
    bool SetString(const char* name, const char* value);

    /// @}

    /**
     * Execute a command string on the server console.
     * @param command Command string (e.g. "changelevel de_dust2").
     *
     * @note Uses IVEngineServer2::ServerCommand. The command is queued and
     *       executed at the next engine opportunity.
     */
    void ExecuteServerCommand(const char* command);

    /// @name Change Listeners
    /// @{

    /**
     * Callback signature for ConVar change notifications.
     * @param name     The ConVar that changed.
     * @param oldValue Previous value as a string.
     * @param newValue New value as a string.
     */
    using ChangeCallback = std::function<void(const char* name, const char* oldValue, const char* newValue)>;

    /**
     * Register a callback that fires whenever any ConVar changes.
     *
     * The global change callback is installed lazily on the first call.
     *
     * @param callback Function to invoke on change.
     * @return Listener ID that can be passed to RemoveChangeListener().
     */
    uint64_t OnChange(ChangeCallback callback);

    /**
     * Remove a previously registered change listener.
     * @param id Listener ID returned by OnChange().
     */
    void RemoveChangeListener(uint64_t id);

    /// @}

    /**
     * Dispatch a change notification to all registered listeners.
     *
     * Called internally by the file-scope global change callback trampoline.
     * Public only because it is accessed from an anonymous-namespace function.
     *
     * @param name     ConVar name.
     * @param oldValue Previous value.
     * @param newValue New value.
     */
    void DispatchChange(const char* name, const char* oldValue, const char* newValue);

private:
    std::unordered_map<uint64_t, ChangeCallback> _changeCallbacks;  ///< Registered listeners.
    uint64_t _nextCallbackId = 1;                                   ///< Monotonically increasing listener ID.
    bool _globalCallbackInstalled = false;  ///< Whether ICvar::InstallGlobalChangeCallback was called.
};

}  // namespace AdminSystem::Sdk
