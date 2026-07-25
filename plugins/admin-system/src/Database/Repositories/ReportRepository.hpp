#pragma once

#include "../Entities/Report.hpp"

#include <functional>

namespace AdminSystem::Database
{

/** Player reports - insert only; the upstream website owns every read and the triage columns. */
class ReportRepository
{
public:
    /** @p onDone reports the write outcome on the game thread. Not fire-and-forget like the other
     *  repositories, because the reporter is told whether their report landed. */
    void CreateAsync(const Report& report, std::function<void(bool ok)> onDone = {});
};

}  // namespace AdminSystem::Database
