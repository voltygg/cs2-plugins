#pragma once

#include "Core/Types.hpp"

#include <Contracts/IMenuSection.hpp>
#include <VoltMod/Core/Signals/Subscription.hpp>

namespace AdminSystem::Core
{

/** The main menu's "admin" entry: shown to admins, opens the admin menu. */
class AdminMenuSection final : public Contracts::IMenuSection
{
public:
    explicit AdminMenuSection(App& app) : _app(app) {}

    /** Offer this to other plugins until this is destroyed. */
    void Publish();

    bool IsVisibleTo(int slot) override;
    bool Open(int slot) override;

private:
    App& _app;
    /** Declared last, so the entry is withdrawn before anything it reaches. */
    VoltMod::Subscription _published;
};

}  // namespace AdminSystem::Core