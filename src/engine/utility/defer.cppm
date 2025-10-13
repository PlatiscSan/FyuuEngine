export module defer;
import std;

export namespace fyuu_engine::util {

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

    auto Lock(std::atomic_flag& mutex) noexcept {
        while (mutex.test_and_set(std::memory_order::acq_rel)) {
            mutex.wait(true, std::memory_order::relaxed);
        }
        return util::Defer(
            [&]() {
                mutex.clear(std::memory_order::release);
                mutex.notify_one();
            }
        );
    }

}