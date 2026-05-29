#pragma once

#include "ActionContext.hpp"

namespace AdminSystem::Admin::Actions
{

/**
 * Theatrical instakill: broadcast, then Slay after a 250 ms beat. Per Phase 0 fallback for
 * the missing particle API — no lightning bolt visual until ParticleService lands; the chat
 * broadcast carries the drama in the meantime. Requires the Fun flag.
 */
extern const Action Smite;

}  // namespace AdminSystem::Admin::Actions
