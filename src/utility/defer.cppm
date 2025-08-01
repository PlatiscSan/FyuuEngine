export module defer;
import std;

export namespace util {

    template <class Func>
    class Defer final {
    private:
        Func m_func;

    public:
        Defer(Func&& func)
            : m_func(std::forward<Func>(func)) {
        }

        Defer(Defer const&) = delete;
        Defer& operator=(Defer const&) = delete;

        Defer(Defer&&) = default;
        Defer& operator=(Defer&&) = default;

        ~Defer() noexcept {
            try { std::invoke(m_func); }
            catch (...) {  }
        }
    };
}