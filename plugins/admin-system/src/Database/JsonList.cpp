#include "JsonList.hpp"

#include <glaze/json.hpp>

namespace AdminSystem::Database
{

std::optional<std::vector<std::string>> ReadJsonList(std::string_view text)
{
    if (text.empty())
        return std::vector<std::string>{};

    std::vector<std::string> values;
    if (glz::read_json(values, text))
        return std::nullopt;
    return values;
}

}  // namespace AdminSystem::Database
