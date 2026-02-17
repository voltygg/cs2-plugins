#pragma once

namespace AdminSystem::Core
{

/**
 * CRTP singleton base. Derived classes inherit via `class Foo : public Singleton<Foo>`
 * and declare `friend class Singleton<Foo>` with a private default constructor.
 * Instance lifetime is static-local (created on first access, destroyed at program exit).
 */
template <typename T>
class Singleton
{
public:
    static T& Instance()
    {
        static T instance;
        return instance;
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

protected:
    Singleton() = default;
    ~Singleton() = default;
};

}  // namespace AdminSystem::Core
