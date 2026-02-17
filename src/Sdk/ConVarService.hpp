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
 * Also provides server command execution via IVEngineServer2.
 */
class ConVarService : public Core::Singleton<ConVarService>
{
public:
    explicit ConVarService(Token) {}

    bool Initialize();

    // Find & read
    std::optional<int> GetInt(const char* name) const;
    std::optional<float> GetFloat(const char* name) const;
    std::optional<std::string> GetString(const char* name) const;
    std::optional<bool> GetBool(const char* name) const;
    bool Exists(const char* name) const;

    // Write
    bool SetInt(const char* name, int value);
    bool SetFloat(const char* name, float value);
    bool SetString(const char* name, const char* value);

    // Server command execution (via IVEngineServer2)
    void ExecuteServerCommand(const char* command);

    // Change listener
    using ChangeCallback = std::function<void(const char* name, const char* oldValue, const char* newValue)>;
    uint64_t OnChange(ChangeCallback callback);
    void RemoveChangeListener(uint64_t id);

    // Called by the global change callback trampoline (in .cpp)
    void DispatchChange(const char* name, const char* oldValue, const char* newValue);

private:
    std::unordered_map<uint64_t, ChangeCallback> _changeCallbacks;
    uint64_t _nextCallbackId = 1;
    bool _globalCallbackInstalled = false;
};

}  // namespace AdminSystem::Sdk
