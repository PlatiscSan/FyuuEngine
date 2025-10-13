export module concurrent_vector;
export import concurrent_container_base;
import std;
import defer;

export namespace fyuu_engine::concurrency {

	template <class T, class Allocator = std::allocator<T>>
	class ConcurrentVector {
	public:
		using value_type = T;
		using allocator_type = Allocator;
		using size_type = typename std::allocator_traits<allocator_type>::size_type;
		using difference_type = typename std::allocator_traits<allocator_type>::difference_type;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = typename std::allocator_traits<allocator_type>::pointer;
		using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;

	private:
		using SharedLock = std::shared_lock<std::shared_mutex>;
		using UniqueLock = std::unique_lock<std::shared_mutex>;
		using UniqueLockAlloc = typename std::allocator_traits<allocator_type>::template rebind_alloc<UniqueLock>;
		using SharedLockAlloc = typename std::allocator_traits<allocator_type>::template rebind_alloc<SharedLock>;
		using UniqueLocks = std::vector<std::unique_lock<std::shared_mutex>, UniqueLockAlloc>;
		using SharedLocks = std::vector<std::shared_lock<std::shared_mutex>, SharedLockAlloc>;

	public:
		class SafeReference {
		private:
			UniqueLock m_lock;
			reference m_ref;

		public:
			SafeReference(reference ref, UniqueLock&& lock)
				:m_ref(ref), m_lock(std::move(lock)) {

			}

			reference Get() noexcept {
				return m_ref;
			}

			operator reference() noexcept {
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
			SharedLock m_lock;
			const_reference m_ref;

		public:
			ConstSafeReference(const_reference ref, SharedLock&& lock)
				:m_ref(ref), m_lock(std::move(lock)) {

			}

			const_reference Get() const noexcept {
				return m_ref;
			}

			operator const_reference() const noexcept {
				return m_ref;
			}

			template <std::convertible_to<value_type> Compatible>
			bool operator==(Compatible&& value) const noexcept {
				return m_ref == value;
			}

		};

	private:
		struct Element {
			mutable Mutex mutex;
			value_type storage;

			template <class... Args>
			Element(Args&&... args)
				: storage(std::forward<Args>(args)...) {

			}

			Element(Element&& other) noexcept
				: storage(std::move(LockedReference(other.storage, other.mutex.Lock()).Get())) {

			}

		};

		using ElementAlloc = typename std::allocator_traits<allocator_type>::template rebind_alloc<Element>;

		Element* m_elements = nullptr;
		std::atomic<size_type> m_size = 0;
		size_type m_capacity = 0;
		mutable Mutex m_mutex;
		std::atomic<size_type> m_writers = 0;
		std::atomic<size_type> m_readers = 0;
		ElementAlloc m_alloc;

		template <class Container, class Value>
		class IteratorBase {
		protected:
			Container* m_vector;
			size_type m_index;

			bool IsEnd() const noexcept {
				return m_index == m_vector->m_size.load(std::memory_order::acquire);
			}

			bool IsBegin() const noexcept {
				return m_index == 0;
			}

		public:
			using iterator_category = std::random_access_iterator_tag;
			using value_type = Value;
			using difference_type = std::ptrdiff_t;
			using pointer = value_type*;
			using const_pointer = value_type*;
			using reference = value_type&;
			using const_reference = value_type const&;

			IteratorBase(Container* vector, size_type index)
				:m_vector(vector), m_index(index) {

			}

			template <class = typename std::enable_if_t<!std::is_const_v<Container>>>
			reference operator*() noexcept {
				return m_vector->m_elements[m_index].storage;
			}

			const_reference operator*() const noexcept {
				return m_vector->m_elements[m_index].storage;
			}

			template <class = typename std::enable_if_t<!std::is_const_v<Container>>>
			pointer operator->() noexcept {
				return &(m_vector->m_elements[m_index].storage);
			}

			const_pointer operator->() const noexcept {
				return &(m_vector->m_elements[m_index].storage);
			}

			difference_type operator-(IteratorBase const& other) const noexcept {
				return static_cast<difference_type>(m_index) - static_cast<difference_type>(other.m_index);
			}

			IteratorBase& operator++() {
				if (!IsEnd()) {
					++m_index;
				}
				return *this;
			}

			IteratorBase operator++(int) {
				IteratorBase tmp = *this;
				++(*this);
				return tmp;
			}

			IteratorBase& operator--() {
				if (!IsBegin()) {
					--m_index;
				}
				return *this;
			}

			IteratorBase operator--(int) {
				IteratorBase tmp = *this;
				--(*this);
				return tmp;
			}

			IteratorBase& operator+=(difference_type n) {
				if (n >= 0) {
					while (n--) {
						++(*this);
					}
				}
				else {
					while (n++) {
						--(*this);
					}
				}
				return *this;
			}

			IteratorBase operator+(difference_type n) const {
				IteratorBase temp = (*this);
				return temp += n;
			}

			friend IteratorBase operator+(difference_type n, IteratorBase const& iter) {
				IteratorBase temp = (*this);
				return temp += n;
			}

			IteratorBase& operator-=(difference_type n) {
				return (*this) += -n;
			}

			IteratorBase operator-(difference_type n) const {
				IteratorBase temp = *this;
				return temp -= n;
			}

			reference operator[](size_type offset) noexcept {
				return *((*this) + offset);
			}

			const_reference operator[](size_type offset) const noexcept {
				return *((*this) + offset);
			}

			bool operator<(IteratorBase const& other) const noexcept {
				return other - (*this) > 0;
			}

			bool operator>(IteratorBase const& other) const noexcept {
				return other < (*this);
			}

			bool operator>=(IteratorBase const& other) const noexcept {
				return !((*this) < other);
			}

			bool operator<=(IteratorBase const& other) const noexcept {
				return !((*this) > other);
			}

			bool operator==(IteratorBase const& other) const noexcept {
				return m_vector == other.m_vector && m_index == other.m_index;
			}

			bool operator!=(IteratorBase const& other) const noexcept {
				return !((*this) == other);
			}

		};

	public:
		class iterator;
		class const_iterator;
		class Modifier;

		class iterator : public IteratorBase<ConcurrentVector, value_type> {
			friend const_iterator;
			friend class Modifier;
		public:
			using Base = IteratorBase<ConcurrentVector, value_type>;
			using Base::Base;

		};

		class const_iterator : public IteratorBase<ConcurrentVector const, value_type const> {
			friend iterator;
			friend class Modifier;
		public:
			using Base = IteratorBase<ConcurrentVector const, value_type const>;
			using Base::Base;

			const_iterator(iterator const& other) noexcept
				: Base(other.m_vector, other.m_index) {
			}
		};

		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	private:
		/// @brief Reserves storage for at least the specified number of elements, reallocating and moving existing elements if necessary.
		/// @param new_cap The new capacity to reserve for the container.
		void ReserveImp(size_type new_cap) {

			/*
			*	Only one writer thread can perform the reallocation.
			*/

			UniqueLock lock = m_mutex.Lock();

			if (new_cap <= m_capacity) {
				return;
			}

			auto new_block = std::allocator_traits<ElementAlloc>::allocate(m_alloc, new_cap);

			if (m_elements) {
				size_type size = m_size.load(std::memory_order::relaxed);
				for (size_type i = 0; i < size; ++i) {
					std::allocator_traits<ElementAlloc>::construct(m_alloc, &new_block[i], std::move(m_elements[i]));
					std::allocator_traits<ElementAlloc>::destroy(m_alloc, &m_elements[i]);
				}

				std::allocator_traits<ElementAlloc>::deallocate(m_alloc, m_elements, m_capacity);
			}

			m_elements = new_block;
			m_capacity = new_cap;

		}

		/// @brief 
		/// @tparam ...Args 
		/// @param ...args 
		/// @return 
		template <class... Args>
		SafeReference EmplaceBackImp(Args&&... args) {

			bool constructed = false;
			size_type constructed_place;
			do {

				if (m_capacity == m_size.load(std::memory_order::acquire)) {
					/*
					*	Only one writer thread can perform the reallocation.
					*/
					ReserveImp(m_capacity == 0 ? 16u : m_capacity * 2u);
				}

				/*
				*	This lock guarantees that no reallocation is performed
				*	when the size is enough
				*/
				auto lock = m_mutex.LockShared();

				/*
				*	Wait until read operation done
				*/

				size_type readers;
				do {
					readers = m_readers.load(std::memory_order::acquire);
					if (readers > 0) {
						m_readers.wait(readers, std::memory_order::relaxed);
					}
				} while (readers > 0);

				/*
				*	now in write mode, no readers can perform operation
				*/

				m_writers.fetch_add(1u, std::memory_order::release);
				util::Defer gc = [this]() {
					m_writers.fetch_sub(1u, std::memory_order::release);
					m_writers.notify_all();
					};

				bool full = false;
				size_type old_size;
				size_type new_size;
				do {
					old_size = m_size.load(std::memory_order::acquire);
					if (old_size == m_capacity) {
						full = true;
						break;
					}
					new_size = old_size + 1;
				} while (!m_size.compare_exchange_weak(
					old_size,
					new_size,
					std::memory_order::release,
					std::memory_order::relaxed));

				if (!full) {

					std::allocator_traits<ElementAlloc>::construct(
						m_alloc,
						&m_elements[old_size],
						std::forward<Args>(args)...
					);
					
					constructed = true;
					constructed_place = old_size;

				}

				/*
				*	reallocate at the beginning if full
				*/

			} while (!constructed);

			return SafeReference(m_elements[constructed_place].storage, m_elements[constructed_place].mutex.Lock());

		}

		/// @brief Removes and returns the last element from the container in a thread-safe manner, or returns an empty optional if the container is empty.
		/// @return An optional containing the removed element if the container was not empty; otherwise, an empty optional.
		std::optional<value_type> PopBackImp() {

			/*
			*	This lock guarantees that no reallocation is performed
			*	when the readers attempt to pop data
			*/
			auto lock = m_mutex.LockShared();

			/*
			*	Wait until writer operation done
			*/

			size_type writers;
			do {
				writers = m_writers.load(std::memory_order::acquire);
				if (writers > 0) {
					m_writers.wait(writers, std::memory_order::relaxed);
				}
			} while (writers > 0);

			/*
			*	now in read mode, no writers can perform operation
			*/

			m_readers.fetch_add(1u, std::memory_order::release);
			util::Defer gc = [this]() {
				m_readers.fetch_sub(1u, std::memory_order::release);
				m_readers.notify_all();
				};


			size_type old_size;
			size_type new_size;
			do {
				old_size = m_size.load(std::memory_order::acquire);
				if (old_size == 0) {
					return std::nullopt;
				}
				new_size = old_size - 1;
			} while (!m_size.compare_exchange_weak(
				old_size,
				new_size,
				std::memory_order::release,
				std::memory_order::relaxed));

			Element& element = m_elements[new_size];
			std::optional<value_type> ret;
			{
				auto element_lock = element.mutex.Lock();
				ret.emplace(std::move(element.storage));
			}
			std::allocator_traits<ElementAlloc>::destroy(
				m_alloc,
				&element
			);

			return ret;

		}

		auto ClearImp() {
			auto lock = m_mutex.Lock();
			size_type count = m_size.load(std::memory_order::acquire);
			for (size_type i = 0; i < count; ++i) {
				std::allocator_traits<ElementAlloc>::destroy(m_alloc, &m_elements[i]);
				m_size.fetch_sub(1u, std::memory_order::release);
			}
			return lock;
		}

	public:
		class View {
		private:
			ConcurrentVector const* m_vector;
			SharedLocks m_locks;

		public:
			View(ConcurrentVector const* vector)
				:m_vector(vector), m_locks(m_vector->m_alloc) {
				m_locks.emplace_back(m_vector->m_mutex.LockShared());
				size_type count = m_vector->m_size.load(std::memory_order::acquire);
				for (std::size_t i = 0; i < count; ++i) {
					m_locks.emplace_back(m_vector->m_elements[i].mutex.LockShared());
				}
			}

			~View() noexcept = default;

			const_iterator end() const {

				size_type count = m_vector->m_size.load(std::memory_order::acquire);
				return const_iterator(m_vector, count);

			}

			const_iterator begin() const {

				size_type count = m_vector->m_size.load(std::memory_order::acquire);
				return const_iterator(m_vector, 0);

			}

			const_iterator cbegin() const {
				return begin();
			}

			const_iterator cend() const {
				return end();
			}

			const_reverse_iterator rbegin() const {
				return const_reverse_iterator(end());
			}

			const_reverse_iterator rend() const {
				return const_reverse_iterator(begin());
			}

			const_reverse_iterator crbegin() const {
				return rbegin();
			}

			const_reverse_iterator crend() const {
				return rend();
			}

		};

		class Modifier {
		private:
			ConcurrentVector* m_vector;
			UniqueLocks m_locks;

		public:
			Modifier(ConcurrentVector* vector)
				:m_vector(vector), m_locks(m_vector->m_alloc) {
				m_locks.emplace_back(m_vector->m_mutex.Lock());
				size_type count = m_vector->m_size.load(std::memory_order::acquire);
				for (std::size_t i = 0; i < count; ++i) {
					m_locks.emplace_back(m_vector->m_elements[i].mutex.Lock());
				}
			}

			iterator begin() {
				return iterator(m_vector, 0);
			}

			iterator end() {
				return iterator(m_vector, m_vector->m_size.load(std::memory_order::acquire));
			}

			const_iterator end() const {

				size_type count = m_vector->m_size.load(std::memory_order::acquire);
				return const_iterator(m_vector, count);

			}

			const_iterator begin() const {

				size_type count = m_vector->m_size.load(std::memory_order::acquire);
				return const_iterator(m_vector, 0);

			}

			const_iterator cbegin() const {
				return begin();
			}

			const_iterator cend() const {
				return end();
			}

			reverse_iterator rbegin() {
				return reverse_iterator(end());
			}

			reverse_iterator rend() {
				return reverse_iterator(begin());
			}

			std::optional<value_type> pop_back() {
				if (m_locks.size() == 1u) {
					// the container is empty;
					return std::nullopt;
				}
				m_locks.pop_back();
				size_type to_pop = m_vector->m_size.fetch_sub(1u, std::memory_order::relaxed);
				std::allocator_traits<ElementAlloc>::destroy(
					m_vector->m_alloc,
					&m_vector->m_elements[to_pop]
				);
			}

			template <class... Args>
			reference emplace_back(Args&&... args) {

				size_type& capacity = m_vector->m_capacity;
				size_type size = m_vector->m_size.load(std::memory_order::relaxed);
				auto& alloc = m_vector->m_alloc;
				auto& elements = m_vector->m_elements;

				if (capacity == size) {

					size_type old_capacity = capacity;
					capacity *= 2;

					auto new_block = std::allocator_traits<ElementAlloc>::allocate(alloc, capacity);

					if (elements) {
						for (size_type i = 0; i < size; ++i) {
							std::allocator_traits<ElementAlloc>::construct(alloc, &new_block[i], std::move(elements[i].storage));
							m_locks[i + 1] = new_block[i].mutex.Lock();
							std::allocator_traits<ElementAlloc>::destroy(alloc, &elements[i]);
						}

						std::allocator_traits<ElementAlloc>::deallocate(alloc, elements, old_capacity);
					}

					elements = new_block;

				}

				std::allocator_traits<ElementAlloc>::construct(
					alloc,
					&elements[size],
					std::forward<Args>(args)...
				);

				m_vector->m_size.fetch_add(1u, std::memory_order::relaxed);

				return elements[size].storage;

			}

			reference push_back(value_type const& value) {
				return emplace_back(value);
			}

			reference push_back(value_type&& value) {
				return emplace_back(value);
			}

			iterator erase(const_iterator pos) {

				if (pos.m_vector != m_vector) {
					throw std::invalid_argument("not the same container");
				}

				size_type current_size = m_locks.size() - 1u;

				if (pos.m_index >= current_size) {
					throw std::out_of_range("erase iterator outside range");
				}

				size_type idx = pos.m_index;

				if (idx != current_size - 1) {
					for (size_type i = idx; i < current_size - 1; ++i) {
						m_vector->m_elements[i].storage =
							std::move(m_vector->m_elements[i + 1].storage);
					}
				}

				auto& last_element = m_vector->m_elements[current_size - 1];
				m_locks.pop_back();
				std::allocator_traits<ElementAlloc>::destroy(
					m_vector->m_alloc,
					&last_element
				);

				m_vector->m_size.store(current_size - 1, std::memory_order_relaxed);

				return iterator(m_vector, idx);

			}

		};

		View LockedView() const {
			return View(this);
		}

		Modifier LockedModifier() {
			return Modifier(this);
		}

		explicit ConcurrentVector(Allocator const& alloc)
			: m_alloc(alloc) {
			ReserveImp(16u);
		}

		ConcurrentVector() noexcept(noexcept(Allocator()))
			: ConcurrentVector(Allocator()) {
		}

		ConcurrentVector(ConcurrentVector const& other) 
			: m_alloc(other.m_alloc) {
			auto locked_other = other.LockedView();
			for (auto const& element : locked_other) {
				EmplaceBackImp(element);
			}
		}

		ConcurrentVector(ConcurrentVector&& other) noexcept 
			: m_alloc(std::move(other.m_alloc)) {
			auto locked_other = other.LockedModifier();
			for (auto& element : locked_other) {
				EmplaceBackImp(std::move(element));
			}
		}

		ConcurrentVector& operator=(ConcurrentVector const& other) {
			if (this != &other) {
				auto this_lock = ClearImp();
				auto locked_other = other.LockedView();
				m_alloc = other.m_alloc;
				for (auto const& element : locked_other) {
					EmplaceBackImp(element);
				}
			}
			return *this;
		}

		ConcurrentVector& operator=(ConcurrentVector&& other) {
			if (this != &other) {
				auto this_lock = ClearImp();
				auto locked_other = other.LockedView();
				m_alloc = std::move(other.m_alloc);
				for (auto& element : locked_other) {
					EmplaceBackImp(std::move(element));
				}
			}
			return *this;
		}

		ConcurrentVector(
			std::initializer_list<T> init,
			Allocator const& alloc = Allocator()
		) : m_alloc(alloc) {
			for (auto& element : init) {
				EmplaceBackImp(element);
			}
		}

		~ConcurrentVector() noexcept {
			auto lock = ClearImp();
			std::allocator_traits<ElementAlloc>::deallocate(m_alloc, m_elements, m_capacity);
		}

		void clear() {
			(void)ClearImp();
		}

		void reserve(size_type new_cap) {
			ReserveImp(new_cap);
		}

		size_type size() const noexcept {
			return m_size.load(std::memory_order::acquire);
		}

		bool empty() const noexcept {
			return size() == 0;
		}

		SafeReference front() {
			auto lock = m_mutex.LockShared();
			if (empty()) {
				throw EmptyContainer();
			}
			Element& element = m_elements[0];
			return SafeReference(element.storage, element.mutex.Lock());
		}

		ConstSafeReference front() const {
			auto lock = m_mutex.LockShared();
			if (empty()) {
				throw EmptyContainer();
			}
			Element& element = m_elements[0];
			return ConstSafeReference(element.storage, element.mutex.LockShared());
		}

		SafeReference back() {
			auto lock = m_mutex.LockShared();
			if (empty()) {
				throw EmptyContainer();
			}
			Element& element = m_elements[m_size - 1];
			return SafeReference(element.storage, element.mutex.Lock());
		}

		ConstSafeReference back() const {
			auto lock = m_mutex.LockShared();
			if (empty()) {
				throw EmptyContainer();
			}
			Element& element = m_elements[m_size - 1];
			return ConstSafeReference(element.storage, element.mutex.LockShared());
		}

		SafeReference at(size_type index) {
			auto lock = m_mutex.LockShared();
			if (index >= m_size.load(std::memory_order::acquire)) {
				throw std::out_of_range("index out of range");
			}
			Element& element = m_elements[index];
			return SafeReference(element.storage, element.mutex.Lock());
		}

		SafeReference operator[](size_type index) {
			return at(index);
		}

		ConstSafeReference at(size_type index) const {
			auto lock = m_mutex.LockShared();
			if (index >= m_size.load(std::memory_order::acquire)) {
				throw std::out_of_range("index out of range");
			}
			Element& element = m_elements[index];
			return SafeReference(element.storage.value(), element.mutex.LockShared());
		}

		ConstSafeReference operator[](size_type index) const {
			return at(index);
		}

		template <class... Args>
		SafeReference emplace_back(Args&&... args) {
			return EmplaceBackImp(std::forward<Args>(args)...);
		}

		SafeReference push_back(value_type const& value) {
			return EmplaceBackImp(value);
		}

		SafeReference push_back(value_type&& value) {
			return EmplaceBackImp(value);
		}

		std::optional<value_type> pop_back() {
			return PopBackImp();
		}

	};
}