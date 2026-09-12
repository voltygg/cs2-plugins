#pragma once

#include "../Entities/Report.hpp"

#include <VoltMod/Database/Api.hpp>
#include <functional>

namespace AdminSystem::Database
{

/** Player reports - insert only; the upstream website owns every read and the triage columns. */
class ReportRepository
{
public:
    explicit ReportRepository(VoltMod::Database& db) : _db(db) {}

    /** @p onDone gets the outcome on the game thread: the reporter is told whether it landed. */
    void CreateAsync(const Report& report, std::function<void(bool ok)> onDone = {});

private:
    VoltMod::Database& _db;
};

}  // namespace AdminSystem::Database
