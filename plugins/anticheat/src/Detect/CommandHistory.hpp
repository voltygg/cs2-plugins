#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <deque>

namespace Anticheat
{

template <class T>
concept HasCommandNumber = requires(const T& entry) {
    { entry.CmdNum } -> std::convertible_to<int32_t>;
};

/** One slot's recent commands, oldest first, at most @p Limit. A client may replay a command
 *  number, so the first entry per number wins and every lookup has a single answer. */
template <HasCommandNumber T, size_t Limit>
class CommandHistory
{
public:
    /** Stores @p entry unless its command number is already held. */
    void Push(const T& entry)
    {
        if (Find(entry.CmdNum))
            return;
        _entries.push_back(entry);
        while (_entries.size() > Limit)
            _entries.pop_front();
    }

    /** The entry numbered @p cmdNum, or nullptr. */
    T* Find(int32_t cmdNum)
    {
        return FindIf([&](const T& entry) { return entry.CmdNum == cmdNum; });
    }

    const T* Find(int32_t cmdNum) const
    {
        return FindIf([&](const T& entry) { return entry.CmdNum == cmdNum; });
    }

    /** The newest entry satisfying @p match, or nullptr. */
    template <class Predicate>
    T* FindIf(Predicate match)
    {
        auto found = std::find_if(_entries.rbegin(), _entries.rend(), match);
        return found == _entries.rend() ? nullptr : &*found;
    }

    template <class Predicate>
    const T* FindIf(Predicate match) const
    {
        auto found = std::find_if(_entries.rbegin(), _entries.rend(), match);
        return found == _entries.rend() ? nullptr : &*found;
    }

    void Clear() { _entries.clear(); }
    bool Empty() const { return _entries.empty(); }
    size_t Size() const { return _entries.size(); }

    /** The most recent command. Undefined when the ring is empty. */
    const T& Newest() const { return _entries.back(); }

    auto begin() const { return _entries.begin(); }
    auto end() const { return _entries.end(); }
    auto rbegin() const { return _entries.rbegin(); }
    auto rend() const { return _entries.rend(); }

private:
    std::deque<T> _entries;
};

}  // namespace Anticheat
