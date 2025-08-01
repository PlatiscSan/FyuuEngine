export module concurrent_container_base;
export import pointer_wrapper;
import std;

export namespace concurrency {

    using SharedLock = std::shared_lock<std::shared_mutex>;
    using UniqueLock = std::unique_lock<std::shared_mutex>;

    struct Mutex {
    private:
        std::shared_mutex m_imp_mutex;

    public:
        SharedLock LockShared() {
            return SharedLock(m_imp_mutex);
        }

        UniqueLock Lock() {
            return UniqueLock(m_imp_mutex);
        }

    };

    class EmptyContainer : public std::exception {
    public:
        EmptyContainer() noexcept
            : std::exception("empty container") {

        }
    };

}