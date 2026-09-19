#pragma once

#include "Core/Types.hpp"

#include <Contracts/IMenuSection.hpp>

namespace AdminSystem::Core
{

/** The main menu's "admin" entry: shown to admins, opens the admin menu. */
class AdminMenuSection final : public Contracts::IMenuSection
{
public:
    explicit AdminMenuSection(App& app) : _app(app) {}

    void Publish();
    /** Called before the managers this delegates to are destroyed. */
    void Unpublish();

    bool IsVisibleTo(int slot) override;
    bool Open(int slot) override;

private:
    App& _app;
};

}  // namespace AdminSystem::Core