#pragma once

#include "../Entities.hpp"

#include <VoltMod/Database/Api.hpp>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace AdminSystem::Database
{

/** The punishments table: bans, voice mutes, text mutes and warnings, read in one query. */
class PunishmentRepository
{
public:
    explicit PunishmentRepository(VoltMod::Database& db) : _db(db) {}

    /** Blocking - load-time only. */
    std::vector<Punishment> FindAllActive();

    /** The same snapshot off-thread; @p onDone runs on the game thread. */
    void FindAllActiveAsync(std::function<void(std::vector<Punishment>)> onDone);

    /** @p onId receives the generated row id on the game thread. */
    void CreateAsync(const Punishment& record, std::function<void(int64_t)> onId = {});

    void RemoveAsync(int64_t recordId, int64_t removedBy, const std::string& reason);

    /** Deactivate every row, of every kind, whose expiry has passed. */
    void ExpireOldAsync();

    /** Active rows of one kind against one player. FIFO, so an earlier create is counted. */
    void CountActiveAsync(PunishType kind, int64_t steamId, std::function<void(int count)> onDone);

    /** Deactivate one kind against one player, as escalation resets warnings. */
    void ClearAsync(PunishType kind, int64_t steamId);

private:
    VoltMod::Database& _db;
};

}  // namespace AdminSystem::Database
