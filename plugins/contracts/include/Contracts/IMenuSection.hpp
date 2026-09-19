#pragma once

#include <string_view>

namespace Contracts
{

/**
 * @brief A main menu entry that opens a plugin's own UI, published with `Exchange.Publish<IMenuSection>(this, id)`.
 *
 * main-menu hides the entry while nothing is published. A vtable change bumps the /N.
 */
struct IMenuSection
{
    static constexpr std::string_view InterfaceName = "cs2plugins.IMenuSection/1";

    /** Whether @p slot sees the entry now. */
    virtual bool IsVisibleTo(int slot) = 0;

    /** Open the section for @p slot. False when refused without telling the player why; the caller replies then. */
    virtual bool Open(int slot) = 0;

protected:
    // Consumers borrow; they never own or delete.
    ~IMenuSection() = default;
};

}  // namespace Contracts
