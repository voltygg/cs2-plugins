#include "command.h"

#include "../utils/string.h"

using namespace std;

namespace commands
{

bool Command::Matches(const string& cmd) const
{
    using utils::String;

    if (String::ToLower(name) == String::ToLower(cmd))
        return true;

    for (const auto& alias : aliases)
    {
        if (String::ToLower(alias) == String::ToLower(cmd))
            return true;
    }

    return false;
}

}  // namespace commands
