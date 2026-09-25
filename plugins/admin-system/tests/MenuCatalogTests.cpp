#include "Admin/Menu/MenuCatalog.hpp"
#include "Punishments/PunishType.hpp"

#include <algorithm>
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <glaze/json.hpp>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace Menu = AdminSystem::Admin::Menu;
using AdminSystem::Punishments::PunishTypes;
using Menu::RowSpec;

static void CollectRows(std::span<const RowSpec> group, std::vector<RowSpec>& rows)
{
    for (const RowSpec& row : group)
    {
        rows.push_back(row);
        CollectRows(row.Children, rows);
    }
}

/** Every row of every table, children included. */
static std::vector<RowSpec> AllRows()
{
    std::vector<RowSpec> rows;
    for (const Menu::TabSpec& tab : Menu::Tabs)
    {
        CollectRows(tab.Rows, rows);
    }
    return rows;
}

/** The translation file as a generic tree: some groups nest further (report.reasons.*). */
static glz::json_t Translations(const std::string& language)
{
    const std::filesystem::path path =
        std::filesystem::path(ADMIN_SYSTEM_DIR) / "translations" / (language + ".json");
    std::ifstream file(path, std::ios::binary);
    REQUIRE_MESSAGE(file.good(), "missing " << path.string());

    std::ostringstream text;
    text << file.rdbuf();

    glz::json_t root;
    const auto error = glz::read_json(root, text.str());
    REQUIRE_MESSAGE(!error, "unparsable " << path.string());
    return root;
}

/** Whether @p dotted resolves to a string, walking one object per segment. */
static bool HasPhrase(const glz::json_t& root, std::string_view dotted)
{
    const glz::json_t* node = &root;
    for (std::size_t start = 0; start <= dotted.size();)
    {
        const std::size_t dot = dotted.find('.', start);
        const std::string segment{dotted.substr(start, dot == std::string_view::npos ? dot : dot - start)};
        if (!node->is_object())
        {
            return false;
        }

        const auto& object = node->get_object();
        const auto entry = object.find(segment);
        if (entry == object.end())
        {
            return false;
        }

        node = &entry->second;
        if (dot == std::string_view::npos)
        {
            break;
        }
        start = dot + 1;
    }
    return node->holds<std::string>();
}

/** The exact icon names panorama/screens/admin_menu/icons.j2 generates. A tab naming anything
 *  else draws no icon at all. */
static constexpr std::string_view IconNames[] = {"punish", "control", "effects", "fun", "map", "chat"};

TEST_CASE("Tabs are indexed by TabId and name a generated icon")
{
    for (std::size_t i = 0; i < Menu::Tabs.size(); ++i)
    {
        CHECK(static_cast<std::size_t>(Menu::Tabs[i].Id) == i);
        CHECK_FALSE(Menu::Tabs[i].LabelKey.empty());
        CHECK_MESSAGE(std::ranges::contains(IconNames, Menu::Tabs[i].Icon),
                      "tab " << Menu::Tabs[i].LabelKey << " names icon '" << Menu::Tabs[i].Icon
                             << "', which icons.j2 does not generate");
    }
}

TEST_CASE("Every row appears exactly once across the catalog")
{
    const std::vector<RowSpec> rows = AllRows();
    std::set<Menu::RowId> seen;
    for (const RowSpec& row : rows)
    {
        CHECK_MESSAGE(seen.insert(row.Id).second, "row " << row.LabelKey << " is listed in two tables");
    }

    // A row that is in no table can never be drawn, and its MakeRow case is dead.
    CHECK(seen.size() == static_cast<std::size_t>(Menu::RowId::MessageColor) + 1);
}

TEST_CASE("No row both names a permission and groups children")
{
    for (const RowSpec& row : AllRows())
    {
        if (row.Permission.empty())
        {
            continue;
        }
        CHECK_MESSAGE(row.Children.empty(), "row " << row.LabelKey
                                                   << " names a permission and also groups children, so its own"
                                                   << " permission would be ignored in favour of theirs");
    }
}

TEST_CASE("The punish card mirrors the punishment type table")
{
    REQUIRE(Menu::PunishCardRows.size() == PunishTypes.size());
    for (std::size_t i = 0; i < PunishTypes.size(); ++i)
    {
        CHECK_MESSAGE(Menu::PunishCardRows[i].Permission == PunishTypes[i].RequiredPermission,
                      "punish row " << Menu::PunishCardRows[i].LabelKey << " is shown on '"
                                    << Menu::PunishCardRows[i].Permission << "' but issued on '"
                                    << PunishTypes[i].RequiredPermission << "'");
    }
}

TEST_CASE("Every catalog label is translated in every language")
{
    std::vector<std::string> keys;
    for (const Menu::TabSpec& tab : Menu::Tabs)
    {
        keys.emplace_back(tab.LabelKey);
    }
    for (const RowSpec& row : AllRows())
    {
        keys.emplace_back(row.LabelKey);
    }

    for (const std::string& language : {std::string("en"), std::string("ru")})
    {
        const glz::json_t phrases = Translations(language);
        for (const std::string& key : keys)
        {
            CHECK_MESSAGE(HasPhrase(phrases, key), language << ".json is missing '" << key << "'");
        }
    }
}
