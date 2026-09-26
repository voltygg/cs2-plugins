#pragma once

#include "Core/Types.hpp"

#include <Contracts/IMenuSection.hpp>
#include <VoltMod/Core/Signals/Subscription.hpp>

namespace AdminSystem::Reports
{

/** The main menu's "report" entry: shown while reports are on, opens the `!report` player picker. */
class ReportMenuSection final : public Contracts::IMenuSection
{
public:
    explicit ReportMenuSection(App& app) : _app(app) {}

    /** Offer this to other plugins until this is destroyed. */
    void Publish();

    bool IsVisibleTo(int slot) override;
    bool Open(int slot) override;

private:
    App& _app;
    /** Declared last, so the entry is withdrawn before anything it reaches. */
    VoltMod::Subscription _published;
};

}  // namespace AdminSystem::Reports
