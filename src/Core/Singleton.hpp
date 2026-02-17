#pragma once

namespace AdminSystem::Core
{

/**
 * CRTP singleton base using the pass-key idiom.
 *
 * Derived classes inherit via `class Foo : public Singleton<Foo>` and declare
 * a public constructor taking Token: `explicit Foo(Token) {}`.
 *
 * No `friend` declaration is needed. `Token` is `protected`, so only
 * `Singleton<T>` and its derivatives can construct one, preventing external
 * instantiation while keeping the derived constructor public.
 *
 * Instance lifetime is static-local: created on first call to Instance(),
 * destroyed at program exit in reverse construction order.
 *
 * @tparam T The derived class type (CRTP parameter).
 *
 * @par Usage
 * @code
 * class MyService : public Core::Singleton<MyService>
 * {
 * public:
 *     explicit MyService(Token) {}
 *     void DoWork();
 * };
 *
 * // Access: MyService::Instance().DoWork();
 * @endcode
 */
template <typename T>
class Singleton
{
public:
    /**
     * @brief Get the singleton instance (created on first call).
     * @return Reference to the single instance of T.
     */
    static T& Instance()
    {
        static T instance{Token{}};
        return instance;
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

protected:
    /**
     * @brief Pass-key token that restricts who can construct T.
     *
     * Only `Singleton<T>` and classes derived from it can access this type,
     * so external code cannot construct T even though T's constructor is public.
     */
    struct Token
    {};
    Singleton() = default;
    ~Singleton() = default;
};

}  // namespace AdminSystem::Core
