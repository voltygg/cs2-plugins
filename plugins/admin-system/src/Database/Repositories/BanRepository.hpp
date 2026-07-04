#pragma once

#include "../Entities/Ban.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace AdminSystem::Database
{

/**
 * Repository for ban records. Reads used at load time block on the database worker; everything
 * on a gameplay path is fire-and-forget (failures logged) or callback-based, so the game thread
 * never waits on the database.
 */
class BanRepository
{
public:
    /** Blocking - load-time only. */
    std::vector<Ban> FindAllActive();

    /** Async snapshot for the periodic cache refresh; @p onDone runs on the game thread. */
    void FindAllActiveAsync(std::function<void(std::vector<Ban>)> onDone);

    /** Async insert; @p onId receives the generated row id on the game thread. */
    void CreateAsync(const Ban& ban, std::function<void(int64_t id)> onId = {});

    void RemoveAsync(int64_t banId, int64_t removedBy, const std::string& reason);
    void ExpireOldAsync();
};

}  // namespace AdminSystem::Database
