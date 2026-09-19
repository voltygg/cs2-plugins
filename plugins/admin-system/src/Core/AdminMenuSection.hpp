#pragma once

#include "Admin/AdminManager.hpp"

#include <Contracts/IMenuSection.hpp>
#include <VoltMod/Runtime.hpp>
#include <functional>
#include <utility>

namespace AdminSystem::Core
{

/** The main menu's "admin" entry: shown to admins, opens the admin menu. */
class AdminMenuSection final : public Contracts::IMenuSection
{
public:
    AdminMenuSection(VoltMod::Runtime& runtime, Admin::AdminManager& admins, std::function<bool(int slot)> open)
        : _rt(runtime), _admins(admins), _open(std::move(open))
    {}

    void Publish();
    /** Called before the managers this delegates to are destroyed. */
    void Unpublish();

    bool IsVisibleTo(int slot) override;
    bool Open(int slot) override;

private:
    VoltMod::Runtime& _rt;
    Admin::AdminManager& _admins;
    std::function<bool(int slot)> _open;
};

}  // namespace AdminSystem::Core
