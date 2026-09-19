#pragma once

#include "Core/Types.hpp"

#include <Contracts/IMenuSection.hpp>

namespace AdminSystem::Reports
{

/** The main menu's "report" entry: shown while reports are on, opens the `!report` player picker. */
class ReportMenuSection final : public Contracts::IMenuSection
{
public:
    explicit ReportMenuSection(App& app) : _app(app) {}

    void Publish();
    /** Called before the managers this delegates to are destroyed. */
    void Unpublish();

    bool IsVisibleTo(int slot) override;
    bool Open(int slot) override;

private:
    App& _app;
};

}  // namespace AdminSystem::Reports
