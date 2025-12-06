export module circular_buffer;
export import concurrent_container_base;
import std;


export namespace fyuu_engine::concurrency {

    template <class T, std::size_t N, bool forcely_heap_alloc = false, class Allocator = std::allocator<T>>
    class CircularBuffer {
    public:
        using value_type = T;
        using allocator_type = Allocator;
        using size_type = typename std::allocator_traits<allocator_type>::size_type;
        using difference_type = typename std::allocator_traits<allocator_type>::difference_type;
        using reference = value_type&;
        using const_reference = value_type const&;
        using pointer = typename std::allocator_traits<allocator_type>::pointer;
        using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;

        class SafeReference {
        private:
            reference m_ref;
            CircularBuffer* m_owner;

        public:
            SafeReference(reference ref, CircularBuffer* owner)
                :m_ref(ref), m_owner(owner) {

            }

            ~SafeReference() noexcept {
                m_owner->m_external_ref_count.fetch_sub(1u, std::memory_order::relaxed);
                m_owner->m_external_ref_count.notify_all();
            }

            operator reference() const noexcept {
                return m_ref;
            }

            template <std::convertible_to<value_type> Compatible>
            SafeReference& operator=(Compatible&& value) noexcept {
                m_ref = value;
                return *this;
            }

            template <std::convertible_to<value_type> Compatible>
            bool operator==(Compatible&& value) const noexcept {
                return m_ref == value;
            }

        };

        class ConstSafeReference {
        private:
            const_reference m_ref;
            CircularBuffer const* m_owner;

        public:
            ConstSafeReference(const_reference ref, CircularBuffer const* owner)
                :m_ref(ref), m_owner(owner) {

            }

            ~ConstSafeReference() noexcept {
                m_owner->m_external_ref_count.fetch_sub(1u, std::memory_order::relaxed);
                m_owner->m_external_ref_count.notify_all();
            }

            operator const_reference () const noexcept {
                return m_ref;
            }

            template <std::convertible_to<value_type> Compatible>
            bool operator==(Compatible&& value) const noexcept {
                return m_ref == value;
            }

        };

        static constexpr std::size_t const Capacity = N;

    private:
        static constexpr std::size_t const kBufferSize = N + 1;
        static constexpr bool UseSOO = !forcely_heap_alloc && (sizeof(T) <= 8u);

        struct HeapArray {
            pointer data;
            allocator_type alloc;

            reference operator[](size_type read_index) noexcept {
                return data[read_index];
            }

            const_reference operator[](size_type read_index) const noexcept {
                return data[read_index];
            }

            HeapArray(allocator_type const& alloc = allocator_type()) noexcept
                : data(nullptr),
                alloc(alloc) {
                data = std::allocator_traits<allocator_type>::allocate(this->alloc, kBufferSize);
            }

            ~HeapArray() noexcept {
                std::allocator_traits<allocator_type>::deallocate(alloc, data, kBufferSize);
            }

        };

        using Storage = std::conditional_t<UseSOO, std::array<value_type, kBufferSize>, HeapArray>;


        /// @brief Waits for the value of an atomic index to change, optionally yielding or waiting efficiently depending on platform support.
        struct Retry {
            bool operator()(std::atomic<size_type>& index) const noexcept {

                bool modified;
                size_type old_index = index.load(std::memory_order::acquire);
                do {
                    modified = index.load(std::memory_order::acquire) != old_index;
                    if (!modified) {
                        index.wait(old_index, std::memory_order::relaxed);
                    }
                } while (!modified);

                return true;

            }
        };

        /// @brief A functor that always indicates no retry is needed.
        struct NoRetry {
            bool operator()(std::atomic<size_type>& read_index) const noexcept {
                return false;
            }
        };

        alignas(std::hardware_destructive_interference_size) std::atomic<size_type> m_read_index = 0u;
        alignas(std::hardware_destructive_interference_size) std::atomic<size_type> m_write_index = 0u;

        /// @brief updating or pending write position
        alignas(std::hardware_destructive_interference_size) std::atomic<size_type> m_pending_write_index = 0u;

        mutable std::atomic<size_type> m_external_ref_count;
        std::atomic<size_type> m_push_pop_count;
        Storage m_storage;

        static void Notify(std::atomic<size_type>& atomic_var) {
            atomic_var.notify_all();
        }

        template <class Func>
        struct Defer {
            Func func;
            ~Defer() noexcept {
                func();
            }
        };

        template <class FailureHandler, class... Args>
        bool EmplaceBackImp(FailureHandler&& handler, Args&&... args) {

            // block until no reference

            size_type external_ref_count = 0;
            do {
                external_ref_count = m_external_ref_count.load(std::memory_order::acquire);
                if (external_ref_count > 0) {
                    m_external_ref_count.wait(external_ref_count, std::memory_order::relaxed);
                }
            } while (external_ref_count > 0);

            m_push_pop_count.fetch_add(1u, std::memory_order::release);
            Defer defer(
                [this]() {
                    m_push_pop_count.fetch_sub(1u, std::memory_order::release);
                    CircularBuffer::Notify(m_push_pop_count);
                }
            );

            // update the buffer with CAS.

            size_type old_write_index = m_write_index.load(std::memory_order_acquire);
            size_type new_write_index;
            do {

                new_write_index = (old_write_index + 1) % kBufferSize;

                // Determine whether the queue is full
                if (new_write_index == m_read_index.load(std::memory_order::acquire)) {
                    if (handler(m_read_index)) {
                        continue;
                    }
                    else {
                        return false;
                    }
                }

            } while (!m_write_index.compare_exchange_weak(
                old_write_index,
                new_write_index,
                std::memory_order::release,
                std::memory_order::relaxed));

            try {
                if constexpr (UseSOO) {
                    new(&m_storage[old_write_index]) value_type(std::forward<Args>(args)...);
                }
                else {
                    std::allocator_traits<allocator_type>::construct(m_storage.alloc, &m_storage[old_write_index], std::forward<Args>(args)...);
                }
            }
            catch (...) {
                m_write_index.store(old_write_index, std::memory_order::release);
                throw;
            }

            std::size_t pending_write_index;
            do {
                pending_write_index = old_write_index;
            } while (!m_pending_write_index.compare_exchange_weak(
                pending_write_index,
                new_write_index,
                std::memory_order_release,
                std::memory_order_relaxed));

            CircularBuffer::Notify(m_pending_write_index);

            return true;

        }

        /// @brief Attempts to remove and return the front element from a concurrent buffer, handling failure cases with a user-provided handler.
        /// @tparam FailureHandler The type of the callable object used to handle pending write situations.
        /// @param handler A callable object that is invoked when a pending write is detected. It should accept the pending write index and return true to retry or false to abort.
        /// @return An optional containing the removed element if successful; std::nullopt if the buffer is empty or the handler indicates to abort.
        template <class FailureHandler>
        std::optional<value_type> PopFrontImp(FailureHandler&& handler) noexcept {

            // block until no reference.

            size_type external_ref_count = 0;
            do {
                external_ref_count = m_external_ref_count.load(std::memory_order::acquire);
                if (external_ref_count > 0) {
                    m_external_ref_count.wait(external_ref_count, std::memory_order::relaxed);
                }
            } while (external_ref_count > 0);

            m_push_pop_count.fetch_add(1u, std::memory_order::release);
            Defer defer(
                [this]() {
                    m_push_pop_count.fetch_sub(1u, std::memory_order::release);
                    CircularBuffer::Notify(m_push_pop_count);
                }
            );

            // update the buffer with CAS.

            size_type old_read_index = m_read_index.load(std::memory_order::acquire);
            do {

                // Determine whether the write index and the read index overlap. If they do, the queue is empty.
                if (old_read_index == m_write_index.load(std::memory_order::acquire)) {
                    return std::nullopt;
                }

                size_type pending_write_index = m_pending_write_index.load(std::memory_order_acquire);

                // Determine whether the data to be read at this time is consistent with tail_update. 
                // If they are consistent, it means that the tail data has not been updated.
                if (pending_write_index != m_write_index.load(std::memory_order::acquire)) {
                    if (handler(m_pending_write_index)) {
                        continue;
                    }
                    else {
                        return std::nullopt;
                    }
                }

            } while (!m_read_index.compare_exchange_weak(
                old_read_index,
                (old_read_index + 1) % kBufferSize,
                std::memory_order::release,
                std::memory_order::relaxed));

            auto temp = std::move(m_storage[old_read_index]);
            if constexpr (UseSOO) {
                m_storage[old_read_index].~value_type();
            }
            else {
                std::allocator_traits<allocator_type>::destroy(m_storage.alloc, &m_storage[old_read_index]);
            }

            CircularBuffer::Notify(m_read_index);

            return temp;

        }

        void WaitForPushAndPop() {
            size_type push_pop_count = 0;
            do {
                push_pop_count = m_push_pop_count.load(std::memory_order::acquire);
                if (push_pop_count > 0) {
                    m_push_pop_count.wait(push_pop_count, std::memory_order::relaxed);
                }
            } while (push_pop_count > 0);

            m_external_ref_count.fetch_add(1u, std::memory_order::release);
        }

        auto ClearImp() {

            // Block all pop and push, we are about to remove all elements.

            WaitForPushAndPop();
            Defer defer(
                [this]() {
                    m_external_ref_count.fetch_sub(1u, std::memory_order::release);
                    CircularBuffer::Notify(m_external_ref_count);
                }
            );

            while (!empty()) {
                size_type read_index = m_read_index.load(std::memory_order::acquire);
                if constexpr (UseSOO) {
                    m_storage[read_index].~T();
                }
                else {
                    std::allocator_traits<allocator_type>::destroy(m_storage.alloc, &m_storage[read_index]);
                }
                m_read_index.store((read_index + 1) % kBufferSize, std::memory_order::release);
            }

            return defer;

        }

        explicit CircularBuffer(Allocator const& alloc, std::true_type /*small_tag*/)
            : m_storage() {
        }

        explicit CircularBuffer(Allocator const& alloc, std::false_type /*large_tag*/)
            : m_storage(alloc) {
        }

        explicit CircularBuffer(CircularBuffer const& other, std::true_type /*small_tag*/)
            : m_storage() {
        }

        explicit CircularBuffer(CircularBuffer const& other, std::false_type /*large_tag*/)
            : m_storage(other.m_storage.alloc) {
        }

        explicit CircularBuffer(CircularBuffer&& other, std::true_type /*small_tag*/)
            : m_storage() {
        }

        explicit CircularBuffer(CircularBuffer&& other, std::false_type /*large_tag*/)
            : m_storage(other.m_storage.alloc) {
        }

        CircularBuffer(size_type count, T const& value, Allocator const& alloc, std::true_type /*small_tag*/)
            : m_storage() {
            Retry retry{};
            for (size_type i = 0; i < (std::min)(count, kBufferSize); i++) {
                EmplaceBackImp(retry, value);
            }

        }

        CircularBuffer(size_type count, T const& value, Allocator const& alloc, std::false_type /*large_tag*/)
            : m_storage(alloc) {
            Retry retry{};
            for (size_type i = 0; i < (std::min)(count, kBufferSize); i++) {
                EmplaceBack(retry, value);
            }

        }

    public:
        explicit CircularBuffer(Allocator const& alloc)
            : CircularBuffer(alloc, std::integral_constant<bool, UseSOO>{}) {

        }

        CircularBuffer()
            : CircularBuffer(Allocator()) {
        }

        CircularBuffer(size_type count, T const& value, Allocator const& alloc = Allocator())
            : CircularBuffer(count, value, alloc, std::integral_constant<bool, UseSOO>{}) {
        }

        size_type size() const noexcept {
            return
                (m_write_index.load(std::memory_order::acquire) - m_read_index.load(std::memory_order::acquire) + kBufferSize) % kBufferSize;
        }

        bool empty() const noexcept {
            return
                m_write_index.load(std::memory_order::acquire) == m_read_index.load(std::memory_order::acquire);
        }

        /// @brief  Checks if the container can hold more elements.
        /// @return true if the container is full, false otherwise.
        bool Full() const noexcept {
            return
                (m_write_index.load(std::memory_order::acquire) + 1) % kBufferSize == m_read_index.load(std::memory_order::acquire);
        }

        CircularBuffer(CircularBuffer const& other) noexcept
            : CircularBuffer(other, std::integral_constant<bool, UseSOO>{}) {

            // Block all pop and push from other, we are about to copy elements

            other.WaitForPushAndPop();
            Defer defer(
                [this]() {
                    other.m_external_ref_count.fetch_sub(1u, std::memory_order::release);
                    CircularBuffer::Notify(other.m_external_ref_count);
                }
            );

            size_type read_index = other.m_read_index.load(0, std::memory_order::acquire);
            m_read_index.store(read_index, std::memory_order::release);
            m_write_index.store(other.m_write_index.load(std::memory_order::relaxed), std::memory_order::relaxed);
            m_pending_write_index.store(other.m_pending_write_index.load(std::memory_order::relaxed), std::memory_order::relaxed);

            if constexpr (!UseSOO) {
                m_storage.alloc = other.m_storage.alloc;
            }

            while (read_index != other.m_write_index) {
                if constexpr (UseSOO) {
                    new(&m_storage[read_index]) T(other.m_storage[read_index]);
                }
                else {
                    std::allocator_traits<allocator_type>::construct(
                        m_storage.alloc, &m_storage[read_index], other.m_storage[read_index]
                    );
                }
                read_index = (read_index + 1) % kBufferSize;
            }

        }

        CircularBuffer(CircularBuffer&& other) noexcept 
            : CircularBuffer(other, std::integral_constant<bool, UseSOO>{}) {

            // Block all pop and push, we are about to move elements

            other.WaitForPushAndPop();
            Defer defer(
                [this]() {
                    other.m_external_ref_count.fetch_sub(1u, std::memory_order::release);
                    CircularBuffer::Notify(other.m_external_ref_count);
                }
            );

            m_read_index.store(other.m_read_index.exchange(0, std::memory_order::relaxed), std::memory_order::relaxed);
            m_write_index.store(other.m_write_index.exchange(0, std::memory_order::relaxed), std::memory_order::relaxed);
            m_pending_write_index.store(other.m_pending_write_index.exchange(0, std::memory_order::relaxed), std::memory_order::relaxed);
            m_storage = std::move(other.m_storage);

        }

        CircularBuffer& operator=(CircularBuffer const& other) {

            if (this != &other) {

                auto defer = ClearImp();

                // Block all pop and push from other, we are about to copy elements

                other.WaitForPushAndPop();
                other.m_external_ref_count.fetch_add(1u, std::memory_order::release);
                Defer defer(
                    [this]() {
                        other.m_external_ref_count.fetch_sub(1u, std::memory_order::release);
                        CircularBuffer::Notify(other.m_external_ref_count);
                    }
                );

                size_type read_index = other.m_read_index.load(0, std::memory_order::acquire);
                m_read_index.store(read_index, std::memory_order::release);
                m_write_index.store(other.m_write_index.load(std::memory_order::relaxed), std::memory_order::relaxed);
                m_pending_write_index.store(other.m_pending_write_index.load(std::memory_order::relaxed), std::memory_order::relaxed);

                if constexpr (!UseSOO) {
                    m_storage.alloc = other.m_storage.alloc;
                }

                while (read_index != other.m_write_index) {
                    if constexpr (UseSOO) {
                        new(&m_storage[read_index]) T(other.m_storage[read_index]);
                    }
                    else {
                        std::allocator_traits<allocator_type>::construct(
                            m_storage.alloc, &m_storage[read_index], other.m_storage[read_index]
                        );
                    }
                    read_index = (read_index + 1) % kBufferSize;
                }


            }
            return *this;
        }

        CircularBuffer& operator=(CircularBuffer&& other) noexcept {

            if (this != &other) {

                auto defer = ClearImp();

                other.WaitForPushAndPop();
                other.m_external_ref_count.fetch_add(1u, std::memory_order::release);
                Defer defer(
                    [this]() {
                        other.m_external_ref_count.fetch_sub(1u, std::memory_order::release);
                        CircularBuffer::Notify(other.m_external_ref_count);
                    }
                );

                m_read_index.store(other.m_read_index.exchange(0, std::memory_order::relaxed), std::memory_order::relaxed);
                m_write_index.store(other.m_write_index.exchange(0, std::memory_order::relaxed), std::memory_order::relaxed);
                m_pending_write_index.store(other.m_pending_write_index.exchange(0, std::memory_order::relaxed), std::memory_order::relaxed);
                m_storage = std::move(other.m_storage);

            }
            return *this;
        }

        void clear() {
            (void)ClearImp();
        }


        ~CircularBuffer() noexcept {
            (void)ClearImp();
        }

        template <class = typename std::enable_if_t<!UseSOO>>
        allocator_type get_allocator() const noexcept {
            return m_storage.alloc;
        }

        template <class... Args>
        void emplace_back(Args&&... args) {
            (void)EmplaceBackImp(Retry(), std::forward<Args>(args)...);
        }

        /// @brief          Appends the given element value to the end of the container.
        ///                 The new element is initialized as a copy of value.            
        /// @param value    the value of the element to append.
        void push_back(T const& value) {
            (void)EmplaceBackImp(Retry(), value);
        }

        /// @brief          Appends the given element value to the end of the container.
        ///                 value is moved into the new element.           
        /// @param value    the value of the element to append.
        void push_back(T&& value) {
            (void)EmplaceBackImp(Retry(), value);
        }

        template <class... Args>
        bool TryEmplaceBack(Args&&... args) {
            return EmplaceBackImp(NoRetry(), std::forward<Args>(args)...);
        }

        /// @brief              Appends the given element value to the end of the container.
        /// @param value        The new element is initialized as a copy of value.
        /// @return             True if the container is not full, otherwise false.
        bool TryPushBack(T const& value) {
            return EmplaceBackImp(NoRetry(), value);
        }

        /// @brief          Appends the given element value to the end of the container.
        ///                 value is moved into the new element.           
        /// @param value    the value of the element to append.
        /// @return         True if the container is not full, otherwise false.
        bool TryPushBack(T&& value) {
            return EmplaceBackImp(NoRetry(), value);
        }

        T pop_front() {
            return std::move(PopFrontImp(Retry()).value());
        }

        std::optional<T> TryPopFront() {
            return PopFrontImp(NoRetry());
        }

        reference& UnsafeFront() noexcept {
            return m_storage[m_read_index.load(std::memory_order::acquire)];
        }

        const_reference UnsafeFront() const noexcept {
            return m_storage[m_read_index.load(std::memory_order::acquire)];
        }

        /// @brief  access front element in the queue safely.
        ///         WARNING! After calling this deadlock will occur when performing any push or pop if the reference is not released
        /// @return reference to front element
        SafeReference front() {
            WaitForPushAndPop();
            return SafeReference(UnsafeFront(), this);
        }

        /// @brief  access front element in the queue safely.
        ///         WARNING! After calling this deadlock will occur when performing any push or pop if the reference is not released
        /// @return reference to front element
        ConstSafeReference front() const {
            WaitForPushAndPop();
            return SafeReference(UnsafeFront(), this);
        }

        reference& UnsafeBack() noexcept {
            return m_storage[(m_write_index.load(std::memory_order::acquire) + kBufferSize - 1) % kBufferSize];
        }

        const_reference UnsafeBack() const noexcept {
            return m_storage[(m_write_index.load(std::memory_order::acquire) + kBufferSize - 1) % kBufferSize];
        }

        /// @brief  access back element in the queue safely.
        ///         WARNING! After calling this deadlock will occur when performing any push or pop if the reference is not released
        /// @return reference to back element
        SafeReference back() {
            WaitForPushAndPop();
            return SafeReference(UnsafeBack(), this);
        }

        /// @brief  access back element in the queue safely.
        ///         WARNING! After calling this deadlock will occur when performing any push or pop if the reference is not released
        /// @return reference to back element
        ConstSafeReference back() const {
            WaitForPushAndPop();
            return SafeReference(UnsafeBack(), this);
        }

        reference UnsafeAt(size_type index) {
            if (index >= size()) {
                throw std::out_of_range("Index out of range");
            }
            size_type read_index = m_read_index.load(std::memory_order_relaxed);
            size_type pos = (read_index + index) % kBufferSize;
            return m_storage[pos];
        }

        const_reference UnsafeAt(size_type index) const {
            if (index >= size()) {
                throw std::out_of_range("Index out of range");
            }
            size_type read_index = m_read_index.load(std::memory_order_relaxed);
            size_type pos = (read_index + index) % kBufferSize;
            return m_storage[pos];
        }

        /// @brief  access element in the queue safely.
        ///         WARNING! After calling this deadlock will occur when performing any push or pop if the reference is not released
        /// @return reference to element
        SafeReference at(size_type index) {
            WaitForPushAndPop();
            return SafeReference(UnsafeAt(index), this);
        }

        /// @brief  access element in the queue safely.
        ///         WARNING! After calling this deadlock will occur when performing any push or pop if the reference is not released
        /// @return reference to element
        ConstSafeReference at(size_type index) const {
            WaitForPushAndPop();
            return ConstSafeReference(UnsafeAt(index), this);
        }

        friend void swap(CircularBuffer& a, CircularBuffer& b) noexcept {

            a.WaitForPushAndPop();
            a.m_external_ref_count.fetch_add(1u, std::memory_order::release);
            Defer defer_a(
                [this]() {
                    a.m_external_ref_count.fetch_sub(1u, std::memory_order::release);
                    CircularBuffer::Notify(a.m_external_ref_count);
                }
            );

            b.WaitForPushAndPop();
            b.m_external_ref_count.fetch_add(1u, std::memory_order::release);
            Defer defer_b(
                [this]() {
                    b.m_external_ref_count.fetch_sub(1u, std::memory_order::release);
                    CircularBuffer::Notify(b.m_external_ref_count);
                }
            );

            auto swap_atomic = [](auto& x, auto& y) {
                auto tmp = x.load();
                x = y.load();
                y = tmp;
                };

            swap_atomic(a.m_read_index, b.m_read_index);
            swap_atomic(a.m_write_index, b.m_write_index);
            swap_atomic(a.m_pending_write_index, b.m_pending_write_index);

            using std::swap;
            swap(a.m_storage, b.m_storage);
        }

    };

}