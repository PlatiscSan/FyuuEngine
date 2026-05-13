module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <atomic>
#include <concepts>
#include <cstddef>
#include <new>
#include <stdexcept>
#include <type_traits> 
#include <utility>
#endif // !defined(__cpp_lib_modules)
export module plastic.bitmap;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
namespace plastic::concurrency {

	export template <std::unsigned_integral T, bool max_align = false>
	class Bitmap {
	public:
		static consteval std::size_t MaxBits() noexcept {
			return sizeof(T) * 8u;
		}

	private:
		struct AlignedAtomic {
			alignas(std::hardware_constructive_interference_size) std::atomic<T> impl;

			constexpr AlignedAtomic() noexcept : impl{ 0 } {}
			constexpr explicit AlignedAtomic(T desired) noexcept : impl{ desired } {}

			// Copy and move constructors transfer the value (not the alignment).
			AlignedAtomic(const AlignedAtomic& other) noexcept
				: impl{ other.impl.load(std::memory_order::relaxed) } {}

			AlignedAtomic(AlignedAtomic&& other) noexcept
				: impl{ other.impl.exchange(0, std::memory_order::relaxed) } {}

			// Copy and move assignment.
			AlignedAtomic& operator=(AlignedAtomic const& other) noexcept {
				if (this != &other) {
					impl.store(other.impl.load(std::memory_order::relaxed),  std::memory_order::relaxed);
				}
				return *this;
			}

			AlignedAtomic& operator=(AlignedAtomic&& other) noexcept {
				if (this != &other) {
					impl.store(other.impl.exchange(0, std::memory_order::relaxed), std::memory_order::relaxed);
				}
				return *this;
			}

			[[nodiscard]] T load(std::memory_order order = std::memory_order::seq_cst) const noexcept {
				return impl.load(order);
			}

			void store(T desired, std::memory_order order = std::memory_order::seq_cst) noexcept {
				impl.store(desired, order);
			}

			[[nodiscard]] T exchange(T desired, std::memory_order order = std::memory_order::seq_cst) noexcept {
				return impl.exchange(desired, order);
			}

			[[nodiscard]] T fetch_or(T arg, std::memory_order order = std::memory_order::seq_cst) noexcept {
				return impl.fetch_or(arg, order);
			}

			[[nodiscard]] T fetch_and(T arg, std::memory_order order = std::memory_order::seq_cst) noexcept {
				return impl.fetch_and(arg, order);
			}

			[[nodiscard]] T fetch_xor(T arg, std::memory_order order = std::memory_order::seq_cst) noexcept {
				return impl.fetch_xor(arg, order);
			}

			operator std::atomic<T>& () noexcept { return impl; }
			operator const std::atomic<T>& () const noexcept { return impl; }
		};

		std::conditional_t<max_align, AlignedAtomic, std::atomic<T>> m_bits;

		static constexpr T Mask(std::size_t pos) noexcept {
			return T{ 1 } << static_cast<T>(pos);
		}

		constexpr void CheckPosition(std::size_t pos) const {
			if (pos >= MaxBits()) {
				throw std::out_of_range("Position exceeds bitmap capacity");
			}
		}

	public:
		constexpr Bitmap() noexcept : m_bits{ 0 } {}

		explicit constexpr Bitmap(T initial_value) noexcept : m_bits{ initial_value } {}

		Bitmap(Bitmap const& other) noexcept
			: m_bits{ other.m_bits.load(std::memory_order::relaxed) } {}

		Bitmap& operator=(Bitmap const& other) noexcept {
			if (this != &other) {
				m_bits.store(other.m_bits.load(std::memory_order::relaxed), std::memory_order::relaxed);
			}
			return *this;
		}

		Bitmap(Bitmap&& other) noexcept
			: m_bits{ other.m_bits.exchange(0u, std::memory_order::relaxed) } {}

		/// Move assignment.
		Bitmap& operator=(Bitmap&& other) noexcept {
			if (this != &other) {
				m_bits.store(other.m_bits.exchange(0u, std::memory_order::relaxed), std::memory_order::relaxed);
			}
			return *this;
		}

		[[nodiscard]] bool Test(std::size_t pos) const {
			CheckPosition(pos);
			return (m_bits.load(std::memory_order::acquire) & Mask(pos)) != 0;
		}

		[[nodiscard]] bool TestAndSet(std::size_t pos) {
			CheckPosition(pos);
			const T mask = Mask(pos);
			const T old = m_bits.fetch_or(mask, std::memory_order::acq_rel);
			return (old & mask) != 0;
		}

		void Set(std::size_t pos) {
			CheckPosition(pos);
			(void)m_bits.fetch_or(Mask(pos), std::memory_order::acq_rel);
		}

		void Clear(std::size_t pos) {
			CheckPosition(pos);
			(void)m_bits.fetch_and(~Mask(pos), std::memory_order::acq_rel);
		}

		[[nodiscard]] bool Flip(std::size_t pos) {
			CheckPosition(pos);
			const T mask = Mask(pos);
			const T old = m_bits.fetch_xor(mask, std::memory_order::acq_rel);
			return (old & mask) != 0;
		}

		[[nodiscard]] T GetValue() const noexcept {
			return m_bits.load(std::memory_order::acquire);
		}

		void Store(T value) noexcept {
			m_bits.store(value, std::memory_order::release);
		}

		[[nodiscard]] T Exchange(T value) noexcept {
			return m_bits.exchange(value, std::memory_order::acq_rel);
		}

		[[nodiscard]] std::size_t Count() const noexcept {
			T value = m_bits.load(std::memory_order::acquire);
			std::size_t count = 0;
			while (value) {
				count += (value & 1);
				value >>= 1;
			}
			return count;
		}

		[[nodiscard]] bool Any() const noexcept {
			return m_bits.load(std::memory_order::acquire) != 0;
		}

		[[nodiscard]] bool None() const noexcept {
			return m_bits.load(std::memory_order::acquire) == 0;
		}

		void Reset() noexcept {
			m_bits.store(0, std::memory_order::release);
		}
	};

} // namespace plastic::concurrency