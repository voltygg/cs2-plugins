#pragma once

// The plugin's one forward-declaration header - mirrors VoltMod's own
// include/VoltMod/Engine/EngineTypes.hpp. Every other header includes what it names; a name
// belongs here only when its owner holds it by value, so the two headers cannot include each
// other without a cycle.

namespace AdminSystem
{

/** App.hpp holds Admin::Actions::ActionDescriptors by value, and ActionDescriptors' declaring
 *  header (Admin/Actions/Descriptors.hpp) names App& in Swap/CallCheck/CancelCheck - both by
 *  reference only, so the forward declaration here is enough for either side. */
struct App;

}  // namespace AdminSystem
