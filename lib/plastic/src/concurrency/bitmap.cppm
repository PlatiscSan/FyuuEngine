// ============================================================================
// bitmap.cppm - Module interface for a concurrent bitmap (bitset) with atomic operations
// ============================================================================
//
// This module provides a Bitmap class template that supports atomic test-and-set,
// test-and-clear, flip, and other operations on individual bits. It is designed
// for lock‑free concurrent access. The underlying storage is an atomic unsigned
// integer, optionally aligned to avoid false sharing.

module; // Global module fragment – includes necessary headers when modules are not supported

#if !defined(__cpp_lib_modules)
// Standard library headers required by the implementation.
#include <atomic>        // std::atomic, std::memory_order, fetch_or, etc.
#include <concepts>      // std::unsigned_integral
#include <cstddef>       // std::size_t
#include <new>           // std::hardware_constructive_interference_size
#include <stdexcept>     // std::out_of_range
#include <type_traits>   // std::conditional_t
#include <utility>       // std::move, std::exchange
#endif // !defined(__cpp_lib_modules)

export module plastic.bitmap;

#if defined(__cpp_lib_modules)
import std;              // Import the entire standard library if module support is available.
// In non‑module builds, the required headers were already included.
#endif

namespace plastic::concurrency {

    // ------------------------------------------------------------------------
    // Bitmap – concurrent bit array with atomic operations
    // ------------------------------------------------------------------------

    /**
     * @brief A fixed‑size bitmap (bitset) that provides atomic operations on individual bits.
     *
     * The bitmap is backed by an unsigned integer of type T (e.g., std::uint32_t, std::uint64_t).
     * All modifying operations are performed atomically with configurable memory ordering.
     * The template parameter `max_align` controls whether the atomic storage is aligned to
     * avoid false sharing (useful when the bitmap is used in highly concurrent scenarios).
     *
     * @tparam T        An unsigned integral type (e.g., unsigned int, std::uint64_t).
     * @tparam max_align If true, the atomic variable is aligned to a cache line boundary.
     */
    export template <std::unsigned_integral T, bool max_align = false>
    class Bitmap {
    public:
        /// Returns the maximum number of bits this bitmap can hold.
        static consteval std::size_t MaxBits() noexcept {
            return sizeof(T) * 8u;
        }

    private:
        // --------------------------------------------------------------------
        // AlignedAtomic – a wrapper around std::atomic<T> with explicit cache‑line alignment.
        // --------------------------------------------------------------------
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
                    impl.store(other.impl.load(std::memory_order::relaxed),
                               std::memory_order::relaxed);
                }
                return *this;
            }

            AlignedAtomic& operator=(AlignedAtomic&& other) noexcept {
                if (this != &other) {
                    impl.store(other.impl.exchange(0, std::memory_order::relaxed),
                               std::memory_order::relaxed);
                }
                return *this;
            }

            // Forward all atomic operations to the underlying impl.
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

            // Implicit conversion to std::atomic<T>& (useful when needed).
            operator std::atomic<T>& () noexcept { return impl; }
            operator const std::atomic<T>& () const noexcept { return impl; }
        };

        // The actual storage: either a plain std::atomic<T> or the cache‑aligned version.
        std::conditional_t<max_align, AlignedAtomic, std::atomic<T>> m_bits;

        /// Helper to create a bitmask with a single bit set at position `pos`.
        static constexpr T Mask(std::size_t pos) noexcept {
            return T{ 1 } << static_cast<T>(pos);
        }

        /// Checks that `pos` is within the valid range; throws std::out_of_range otherwise.
        constexpr void CheckPosition(std::size_t pos) const {
            if (pos >= MaxBits()) {
                throw std::out_of_range("Position exceeds bitmap capacity");
            }
        }

    public:
        // --------------------------------------------------------------------
        // Construction / assignment
        // --------------------------------------------------------------------

        /// Default constructor – initializes all bits to 0.
        constexpr Bitmap() noexcept : m_bits{ 0 } {}

        /// Construct from an initial integer value (each bit reflects the integer's bits).
        explicit constexpr Bitmap(T initial_value) noexcept : m_bits{ initial_value } {}

        /// Copy constructor – copies the underlying value (atomic load with relaxed ordering).
        Bitmap(Bitmap const& other) noexcept
            : m_bits{ other.m_bits.load(std::memory_order::relaxed) } {}

        /// Copy assignment.
        Bitmap& operator=(Bitmap const& other) noexcept {
            if (this != &other) {
                m_bits.store(other.m_bits.load(std::memory_order::relaxed),
                             std::memory_order::relaxed);
            }
            return *this;
        }

        /// Move constructor – transfers the value, leaving the source reset to 0.
        Bitmap(Bitmap&& other) noexcept
            : m_bits{ other.m_bits.exchange(0u, std::memory_order::relaxed) } {}

        /// Move assignment.
        Bitmap& operator=(Bitmap&& other) noexcept {
            if (this != &other) {
                m_bits.store(other.m_bits.exchange(0u, std::memory_order::relaxed),
                             std::memory_order::relaxed);
            }
            return *this;
        }

        // --------------------------------------------------------------------
        // Bit operations (atomic)
        // --------------------------------------------------------------------

        /**
         * @brief Tests whether the bit at position `pos` is set.
         * @param pos Bit index (0‑based, must be < MaxBits()).
         * @return true if the bit is 1, false otherwise.
         * @throws std::out_of_range if `pos` is invalid.
         */
        [[nodiscard]] bool Test(std::size_t pos) const {
            CheckPosition(pos);
            return (m_bits.load(std::memory_order::acquire) & Mask(pos)) != 0;
        }

        /**
         * @brief Atomically sets the bit at `pos` and returns its previous value.
         * @param pos Bit index.
         * @return true if the bit was already set, false if it was clear.
         * @throws std::out_of_range if `pos` is invalid.
         */
        [[nodiscard]] bool TestAndSet(std::size_t pos) {
            CheckPosition(pos);
            const T mask = Mask(pos);
            const T old = m_bits.fetch_or(mask, std::memory_order::acq_rel);
            return (old & mask) != 0;
        }

        /**
         * @brief Atomically sets the bit at `pos` to 1.
         * @param pos Bit index.
         * @throws std::out_of_range if `pos` is invalid.
         */
        void Set(std::size_t pos) {
            CheckPosition(pos);
            (void)m_bits.fetch_or(Mask(pos), std::memory_order::acq_rel);
        }

        /**
         * @brief Atomically clears the bit at `pos` (sets it to 0).
         * @param pos Bit index.
         * @throws std::out_of_range if `pos` is invalid.
         */
        void Clear(std::size_t pos) {
            CheckPosition(pos);
            (void)m_bits.fetch_and(~Mask(pos), std::memory_order::acq_rel);
        }

        /**
         * @brief Atomically flips the bit at `pos` (0→1, 1→0) and returns its previous value.
         * @param pos Bit index.
         * @return true if the bit was previously set, false if it was clear.
         * @throws std::out_of_range if `pos` is invalid.
         */
        [[nodiscard]] bool Flip(std::size_t pos) {
            CheckPosition(pos);
            const T mask = Mask(pos);
            const T old = m_bits.fetch_xor(mask, std::memory_order::acq_rel);
            return (old & mask) != 0;
        }

        // --------------------------------------------------------------------
        // Bulk operations
        // --------------------------------------------------------------------

        /// Returns the entire bitmap as an integer value (atomic acquire load).
        [[nodiscard]] T GetValue() const noexcept {
            return m_bits.load(std::memory_order::acquire);
        }

        /// Stores a new integer value into the bitmap (atomic release store).
        void Store(T value) noexcept {
            m_bits.store(value, std::memory_order::release);
        }

        /// Atomically replaces the current value with `value` and returns the old value.
        [[nodiscard]] T Exchange(T value) noexcept {
            return m_bits.exchange(value, std::memory_order::acq_rel);
        }

        /// Counts the number of set bits (popcount) in the current value.
        [[nodiscard]] std::size_t Count() const noexcept {
            T value = m_bits.load(std::memory_order::acquire);
            std::size_t count = 0;
            while (value) {
                count += (value & 1);
                value >>= 1;
            }
            return count;
        }

        /// Returns true if any bit is set.
        [[nodiscard]] bool Any() const noexcept {
            return m_bits.load(std::memory_order::acquire) != 0;
        }

        /// Returns true if no bit is set.
        [[nodiscard]] bool None() const noexcept {
            return m_bits.load(std::memory_order::acquire) == 0;
        }

        /// Resets all bits to 0.
        void Reset() noexcept {
            m_bits.store(0, std::memory_order::release);
        }
    };

} // namespace plastic::concurrency