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

    class Lock {
    private:
        std::atomic_flag& m_mutex;

    public:
        Lock(std::atomic_flag& mutex)
            : m_mutex(mutex) {
            while (m_mutex.test_and_set(std::memory_order::acq_rel)) {
                m_mutex.wait(true, std::memory_order::relaxed);
            }
        }
        ~Lock() noexcept {
            m_mutex.clear(std::memory_order::release);
            m_mutex.notify_one();
        }
    };

}