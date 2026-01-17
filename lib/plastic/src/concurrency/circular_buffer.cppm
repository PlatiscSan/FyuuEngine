export module plastic.circular_buffer;
import std;

namespace plastic::concurrency {

	export template <
		class T, 
		std::size_t Capacity
	> class CircularBuffer {
	public:
		using size_type = std::size_t;
		using value_type = T;
		using reference = T&;
		using const_reference = T const&;
		using pointer = T*;
		using const_pointer = T const*;

		static_assert(Capacity > size_type(0), "Capacity must be greater than 0");

	private:
		static constexpr size_type SlotCount = Capacity + 1;
		struct Slot {
			std::array<std::byte, sizeof(T)> data;
			std::atomic_flag valid_flag{};

			T* AsObject() noexcept {
				return reinterpret_cast<T*>(&data);
			}

			void Deconstruct() noexcept {
				AsObject()->~T();
			}

			template <class... Args>
			void Construct(Args&&... args) {
				new(&data) T(std::forward<Args>(args)...);
			}

		};

		std::array<Slot, SlotCount> m_slots;

		alignas(std::hardware_destructive_interference_size) std::atomic<size_type> m_read_pos;
		alignas(std::hardware_destructive_interference_size) std::atomic<size_type> m_write_pos;
		alignas(std::hardware_destructive_interference_size) std::atomic<size_type> m_committed;

	public:
		CircularBuffer() = default;

		CircularBuffer(std::span<T const> view) {
			for (size_type i = 0; i < std::min(view.size(), Capacity); ++i) {
				emplace_back(view[i]);
			}
		}

		~CircularBuffer() noexcept {
			while (!empty()) {
				(void)pop_front();
			}
		}

		bool empty() const noexcept {
			return 
				m_read_pos.load(std::memory_order::relaxed) == m_write_pos.load(std::memory_order::relaxed);
		}

		size_type size() const noexcept {
			return
				(m_write_pos.load(std::memory_order::relaxed) - m_read_pos.load(std::memory_order::relaxed) + SlotCount) % SlotCount;
		}

		size_type capacity() const noexcept {
			return Capacity;
		}

		template <class... Args>
		bool emplace_back(Args&&... args) {

			size_type expected_pos = m_write_pos.load(std::memory_order::acquire);
			size_type desired_pos;
			do {

				desired_pos = (expected_pos + 1) % SlotCount;
				if (desired_pos == m_read_pos.load(std::memory_order::acquire)) {
					/*
					*	buffer full;
					*/
					return false;
				}

			} while (!m_write_pos.compare_exchange_weak(
				expected_pos,
				desired_pos,
				std::memory_order::release,
				std::memory_order::relaxed));

			m_write_pos.notify_all();

			try {
				Slot& slot = m_slots[expected_pos];
				slot.Construct(std::forward<Args>(args)...);

				size_type expected_committed;
				do {
					expected_committed = expected_pos;
				} while (!m_committed.compare_exchange_weak(
					expected_committed,
					desired_pos,
					std::memory_order::release,
					std::memory_order::relaxed));

				slot.valid_flag.test_and_set(std::memory_order::relaxed);

				return true;

			}
			catch (std::exception const&) {

				size_type expected_committed;
				do {
					expected_committed = expected_pos;
				} while (!m_committed.compare_exchange_weak(
					expected_committed,
					desired_pos,
					std::memory_order::release,
					std::memory_order::relaxed));

				throw;

			}

		}

		bool push_back(value_type const& val) {
			return emplace_back(val);
		}

		bool push_back(value_type&& val) {
			return emplace_back(std::move(val));
		}

		std::optional<value_type> pop_front(bool wait_for_element = false) {

			size_type expected_pos = m_read_pos.load(std::memory_order::acquire);
			size_type desired_pos;
			bool cas_done = false;

			do {

				if (expected_pos == m_write_pos.load(std::memory_order::acquire)) {
					/*
					*	empty buffer
					*/
					if (wait_for_element) {
						m_write_pos.wait(expected_pos, std::memory_order::relaxed);
						continue;
					}
					return std::nullopt;
				}

				size_type committed = m_committed.load(std::memory_order::acquire);

				if (committed != m_write_pos.load(std::memory_order::acquire)) {
					/*
					*	the element is being constructed
					*/
					continue;
				}

				desired_pos = (expected_pos + 1) % SlotCount;

				cas_done = m_read_pos.compare_exchange_weak(
					expected_pos,
					desired_pos,
					std::memory_order::release,
					std::memory_order::relaxed
				);

				if (!m_slots[expected_pos].valid_flag.test(std::memory_order::relaxed)) {
					/*
					*	bad construction, advance expected_pos and retry
					*/
					cas_done = false;
				}

			} while (!cas_done);

			Slot& slot = m_slots[expected_pos];
			std::optional<value_type> popped(std::move(*(slot.AsObject())));
			slot.Deconstruct();

			return popped;

		}

	};

}