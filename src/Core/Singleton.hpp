#pragma once

namespace AdminSystem::Core
{

/**
 * CRTP singleton base using the pass-key idiom.
 * Derived classes inherit via `class Foo : public Singleton<Foo>` and declare
 * a public constructor taking Token: `explicit Foo(Token) {}`.
 * No friend declaration needed — Token is protected, so only Singleton<T> and
 * its derivatives can construct one, preventing external instantiation.
 * Instance lifetime is static-local (created on first access, destroyed at program exit).
 */
template <typename T>
class Singleton
{
public:
    static T& Instance()
    {
        static T instance{Token{}};
        return instance;
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

protected:
    // Empty struct used as a construction token to restrict instantiation
    struct Token
    {};
    Singleton() = default;
    ~Singleton() = default;
};

}  // namespace AdminSystem::Core
