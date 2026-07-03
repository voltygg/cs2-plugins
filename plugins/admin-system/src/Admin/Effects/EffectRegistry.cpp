#include "EffectRegistry.hpp"

#include "../Menu/MenuHelpers.hpp"
#include "Descriptors.hpp"

namespace AdminSystem::Admin::Effects
{

namespace
{
using CS2Kit::Menu::MenuBuilder;
namespace Menu = AdminSystem::Admin::Menu;

// Descriptors are namespace-scope globals, so capturing their address is safe for the process.
EffectEntry MakeEntry(const Effect& effect)
{
    return {[e = &effect](MenuBuilder& builder, int admin, int target) {
        Menu::AddEffectToggleRow(builder, admin, target, *e);
    }};
}

EffectEntry MakeEntry(const ParamEffect& effect)
{
    return {[e = &effect](MenuBuilder& builder, int admin, int target) {
        Menu::AddEffectSubmenuRow(builder, admin, target, *e);
    }};
}
}  // namespace

const std::vector<EffectEntry>& EffectRegistry()
{
    // Lazy function-local static: built on first menu open, after all descriptor globals exist.
    // Hide is intentionally absent - it is a self-only Control row + !hide command, not auto-listed.
    static const std::vector<EffectEntry> registry = [] {
        std::vector<EffectEntry> entries;
        entries.push_back(MakeEntry(Ghost));
        entries.push_back(MakeEntry(Disco));
        entries.push_back(MakeEntry(Wallhack));
        entries.push_back(MakeEntry(Model));
        return entries;
    }();
    return registry;
}

}  // namespace AdminSystem::Admin::Effects
