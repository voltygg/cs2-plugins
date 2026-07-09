#include "StatusCommand.hpp"

#include "../Admin/AdminManager.hpp"
#include "Config.hpp"
#include "Managers.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <nlohmann/json.hpp>
#include <string_view>
#include <tier0/dbg.h>
#include <tier1/convar.h>

using CS2Kit::Core::Engine;

namespace AdminSystem::Core
{

namespace
{

bool IsHealthy()
{
    const auto& report = Engine().LoadReport;
    for (const auto& stage : report.Stages())
        if (stage.Status == CS2Kit::StageStatus::Failed)
            return false;
    return report.IsOk("Database");
}

void PrintStatus(const CCommand& args)
{
    const bool asJson = args.ArgC() > 1 && std::string_view(args.Arg(1)) == "json";

    auto status = Engine().Status.BuildJson();
    status["plugin"] = "admin-system";
    status["healthy"] = IsHealthy();

    if (asJson)
    {
        // Single marker-prefixed line so RCON tooling can find it amid console noise.
        Msg("STATUS_JSON %s\n", status.dump().c_str());
        return;
    }

    Msg("=== admin-system status (healthy: %s) ===\n%s\n", IsHealthy() ? "yes" : "no",
        Engine().Status.BuildText().c_str());
}

}  // namespace

StatusCommand::StatusCommand()
    : _command("admin_status", "Report plugin health; 'admin_status json' emits a machine-readable STATUS_JSON line.",
               [](const CCommand& args) { PrintStatus(args); })
{
    auto& status = Engine().Status;

    status.RegisterSection("db", [this] {
        return nlohmann::json{{"connected", Engine().LoadReport.IsOk("Database")},
                              {"migrationVersion", Migration.CurrentVersion},
                              {"migrationsApplied", Migration.Applied}};
    });

    status.RegisterSection("admins", [] {
        return nlohmann::json{{"cached", App().Admins.AdminCount()}, {"groups", App().Admins.GroupCount()}};
    });

    status.RegisterSection("commands", [] { return nlohmann::json{{"registered", Engine().Commands.Count()}}; });

    status.RegisterSection("server", [] {
        const auto& server = App().Config.GetServer();
        return nlohmann::json{{"tag", server.tag}, {"name", server.name}};
    });
}

}  // namespace AdminSystem::Core
