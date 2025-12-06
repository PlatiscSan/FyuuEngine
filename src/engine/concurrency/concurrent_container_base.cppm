export module concurrent_container_base;
export import pointer_wrapper;
import std;

export namespace fyuu_engine::concurrency {

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

    template <class Reference, class GC>
    struct LockedReference {
        Reference& reference;
        GC lock;

        LockedReference(Reference& reference, GC&& lock)
            : reference(reference), lock(std::move(lock)) {

        }

        Reference& Get() const noexcept {
            return reference;
        }

    };

    template <class Key>
    class ImmutableKey {
    private:
        Key m_key;

    public:
        template <class... Args>
        ImmutableKey(Args&&... args)
            : m_key(std::forward<Args>(args)...) {
        }

        ImmutableKey(ImmutableKey const&) = default;
        ImmutableKey& operator=(ImmutableKey const&) = delete;

        ImmutableKey(ImmutableKey&& other) noexcept
            : m_key(std::move(other.m_key)) {

        }

        ImmutableKey& operator=(ImmutableKey&& other) noexcept {
            if (this != &other) {
                m_key = std::move(other.m_key);
            }
            return *this;
        }

        Key const& Get() const noexcept {
            return m_key;
        }
        operator Key const& () const noexcept {
            return m_key;
        }

        bool operator==(ImmutableKey const& other) const {
            return m_key == other.m_key;
        }

    };

}