// ============================================================================
// resource.cppm - Module interface for resource management utilities
// ============================================================================
//
// This module provides two smart resource wrappers:
//   - UniqueResource<T, GC> : a move‑only type that owns a resource and applies
//     a garbage collector (deleter/finalizer) upon destruction or reset.
//   - SharedResource<T, GC> : a reference‑counted shared resource, similar to
//     std::shared_ptr, but with support for custom garbage collection and
//     placement construction (allowing custom allocators or control block
//     placement). It uses a polymorphic control block to handle both heap‑
//     allocated and externally provided storage.

module; // Global module fragment – includes necessary headers when modules are not supported
#include <version>
#if !defined(__cpp_lib_modules)
// Standard library headers required by the implementation.
#include <cstddef>       // std::size_t, std::ptrdiff_t
#include <utility>       // std::forward, std::exchange
#include <optional>      // std::optional
#include <atomic>        // std::atomic, std::atomic_size_t, std::memory_order
#include <new>           // std::hardware_destructive_interference_size
#include <type_traits>   // std::conditional_t, std::is_const_v, std::remove_reference_t, std::add_lvalue_reference_t
#include <concepts>      // std::convertible_to
#include <compare>       // std::strong_ordering
#endif // !defined(__cpp_lib_modules)

export module plastic.resource;

#if defined(__cpp_lib_modules)
import std;               // Import the entire standard library if module support is available.
// In non‑module builds, the required headers were already included.
#endif

// Imports from other plastic modules (they are assumed to provide traditional
// headers when modules are not supported, but we do not handle them here).
export import plastic.disable_copy;
import plastic.sealed_polymorphism;

namespace plastic::utility {

	// ------------------------------------------------------------------------
	// Helper metafunctions (details namespace)
	// ------------------------------------------------------------------------
	namespace details {

		// PointerTraits: detects whether a type is a pointer (including cv‑qualified)
		// and provides associated types.
		template <class U>
		struct PointerTraits {
			static constexpr bool is_pointer = false;
			using element_type = void;
			using pointer = U*;
			using const_pointer = const U*;
		};

		template <class U>
		struct PointerTraits<U*> {
			static constexpr bool is_pointer = true;
			using element_type = U;
			using pointer = U*;
			using const_pointer = const U*;
		};

		template <class U>
		struct PointerTraits<U* const> {
			static constexpr bool is_pointer = true;
			using element_type = U;
			using pointer = U* const;
			using const_pointer = const U* const;
		};

		template <class U>
		struct PointerTraits<U* volatile> {
			static constexpr bool is_pointer = true;
			using element_type = U;
			using pointer = U* volatile;
			using const_pointer = const U* volatile;
		};

		template <class U>
		struct PointerTraits<U* const volatile> {
			static constexpr bool is_pointer = true;
			using element_type = U;
			using pointer = U* const volatile;
			using const_pointer = const U* const volatile;
		};

		// Empty garbage collector functor (does nothing).
		struct EmptyGC {
			template <class T>
			void operator()(T&&) {}
		};

	} // namespace details

	// ------------------------------------------------------------------------
	// UniqueResource – move‑only resource owner with a garbage collector
	// ------------------------------------------------------------------------

	/**
	 * @brief A move‑only wrapper that owns a resource and applies a garbage
	 *        collector (deleter/finalizer) when the resource is released.
	 *
	 * @tparam T The type of the managed resource (e.g., a file handle, pointer,
	 *           or any movable type).
	 * @tparam GC The garbage collector functor type. It must be callable as
	 *            `void operator()(T)` and is invoked when the resource is
	 *            reset or the UniqueResource is destroyed. Defaults to EmptyGC
	 *            (no‑op).
	 */
	export template <class T, class GC = details::EmptyGC>
	class UniqueResource final : public DisableCopy {
	private:
		std::optional<T> m_impl;   ///< Holds the resource value (empty if no resource).
		GC m_gc;                    ///< Garbage collector instance.

	public:
		/// Default constructor – creates an empty resource.
		UniqueResource() noexcept : m_impl(), m_gc() {}

		/**
		 * @brief Constructs a UniqueResource from a resource convertible to T.
		 * @tparam Resource A type convertible to T.
		 * @param rc The resource to take ownership of.
		 * @param collector The garbage collector to use (default‑constructed if omitted).
		 */
		template <std::convertible_to<T> Resource>
		UniqueResource(Resource&& rc, GC const& collector = GC())
			: m_impl(std::in_place, std::forward<Resource>(rc)), m_gc(collector) {}

		/// @overload
		UniqueResource(T const& rc, GC const& collector = GC())
			: m_impl(std::in_place, rc), m_gc(collector) {}

		/// @overload
		UniqueResource(T&& rc, GC const& collector = GC())
			: m_impl(std::in_place, std::move(rc)), m_gc(collector) {}

		// Copy operations are deleted (via DisableCopy).
		UniqueResource(UniqueResource const&) = delete;
		UniqueResource& operator=(UniqueResource const&) = delete;

		/// Move constructor – transfers ownership and garbage collector.
		UniqueResource(UniqueResource&& other) noexcept
			: m_impl(std::move(other.m_impl)), m_gc(std::move(other.m_gc)) {
			other.m_impl.reset();
		}

		/// Move assignment – releases current resource (if any) and moves from other.
		UniqueResource& operator=(UniqueResource&& other) noexcept {
			if (this != &other) {
				if (m_impl) {
					T moved = std::move(*m_impl);
					m_gc(moved);                     // finalize current resource
				}
				m_impl = std::move(other.m_impl);
				m_gc = std::move(other.m_gc);
				other.m_impl.reset();
			}
			return *this;
		}

		/// Destructor – releases the resource (if any) by moving it into the garbage collector.
		~UniqueResource() noexcept {
			Reset();
		}

		/**
		 * @brief Explicitly releases the owned resource (invokes the garbage collector).
		 * After this call, the UniqueResource becomes empty.
		 */
		void Reset() noexcept {
			if (m_impl) {
				T moved = std::move(*m_impl);
				m_impl.reset();
				m_gc(moved);
			}
		}

		/// Returns a reference to the garbage collector (const‑qualified according to self).
		template <class Self>
		auto GetGC(this Self&& self) noexcept
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, GC const&, GC&> {
			return self.m_gc;
		}

		/// Returns a reference to the managed resource (const‑qualified accordingly).
		template <class Self>
		auto Get(this Self&& self) noexcept
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, T const&, T&> {
			return *self.m_impl;
		}

		/// Arrow operator for when T is not a pointer – returns pointer to the resource.
		template <class Self>
		auto operator->(this Self&& self) noexcept
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, T const*, T*>
			requires (!details::PointerTraits<T>::is_pointer) {
			return &self.Get();
		}

		/// Arrow operator for when T is a pointer – returns the raw pointer itself.
		template <class Self>
		auto operator->(this Self&& self) noexcept
			-> std::conditional_t<
				std::is_const_v<std::remove_reference_t<Self>>,
				typename details::PointerTraits<T>::const_pointer,
				typename details::PointerTraits<T>::pointer>
			requires (details::PointerTraits<T>::is_pointer) {
			return self.Get();
		}

		/// Dereference operator – returns a reference to the managed resource.
		template <class Self>
		auto operator*(this Self&& self) noexcept
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, T const&, T&> {
			return self.Get();
		}

		/// Implicit conversion to T&.
		operator T& () noexcept { return *m_impl; }
		/// Implicit conversion to T const&.
		operator T const& () const noexcept { return *m_impl; }

		/// Boolean conversion – true if a resource is held.
		explicit operator bool() const noexcept { return m_impl.has_value(); }

		/**
		 * @brief Dereference operator for when T is a pointer – returns a reference to the pointee.
		 * This is only enabled when T is a pointer type.
		 */
		template <class Self, class U = T>
		auto operator*(this Self&& self) noexcept
			-> std::add_lvalue_reference_t<typename details::PointerTraits<U>::element_type>
			requires details::PointerTraits<U>::is_pointer {
			return *self.m_impl.value();
		}

		/// Defaulted three‑way comparison (compares the held optional values).
		bool operator<=>(UniqueResource const& other) const noexcept = default;
	};

	// ------------------------------------------------------------------------
	// SharedResource – reference‑counted shared resource with polymorphic control block
	// ------------------------------------------------------------------------

	namespace details {

		// Forward declarations of control block types.
		template <class T, class GC> struct DefaultHeapControlBlock;
		template <class T, class GC> struct CustomControlBlock;

		/**
		 * @brief Base class for control blocks used by SharedResource.
		 * It is a polymorphic base (via PolymorphicBase) that holds the reference count
		 * and the actual resource (as a UniqueResource). The derived classes implement
		 * the destruction policy (heap deletion vs. manual destruction).
		 */
		template <class T, class GC>
		struct BaseControlBlock
			: public PolymorphicBase<std::size_t, DefaultHeapControlBlock<T, GC>, CustomControlBlock<T, GC>> {

			// Align to avoid false sharing.
			alignas(std::hardware_destructive_interference_size) std::atomic_size_t m_ref_count;
			UniqueResource<T, GC> m_impl;   ///< The managed resource (with its own GC).

			/**
			 * @brief Constructs a BaseControlBlock.
			 * @tparam Self The actual derived type (used for PolymorphicBase construction).
			 * @tparam Resource Type convertible to T.
			 * @param self Pointer to the derived object.
			 * @param rc Resource to take ownership of.
			 * @param collector Garbage collector for the resource.
			 */
			template <class Self, std::convertible_to<T> Resource>
			BaseControlBlock(Self* self, Resource&& rc, GC const& collector = GC())
				: PolymorphicBase<std::size_t, DefaultHeapControlBlock<T, GC>, CustomControlBlock<T, GC>>(self),
				  m_ref_count(1u),
				  m_impl(std::forward<Resource>(rc), collector) {}

			/// Increments the reference count (relaxed ordering).
			void AddReference() noexcept {
				m_ref_count.fetch_add(1, std::memory_order::relaxed);
			}

			/**
			 * @brief Decrements the reference count and, if it reaches zero,
			 *        invokes the appropriate destruction policy via virtual dispatch.
			 */
			void Release() noexcept {
				this->Visit([](auto* derived) { derived->Release(); });
			}
		};

		/**
		 * @brief Control block for resources allocated on the heap with `new`.
		 * When the last reference is released, it calls `delete this`.
		 */
		template <class T, class GC>
		struct DefaultHeapControlBlock : public BaseControlBlock<T, GC> {
			template <std::convertible_to<T> Resource>
			DefaultHeapControlBlock(Resource&& rc, GC const& collector = GC())
				: BaseControlBlock<T, GC>(this, std::forward<Resource>(rc), collector) {}

			void Release() noexcept {
				if (this->m_ref_count.fetch_sub(1, std::memory_order::acq_rel) == 1u) {
					delete this;
				}
			}
		};

		/**
		 * @brief Control block for resources placed in user‑provided storage (via placement new).
		 * When the last reference is released, it explicitly calls the destructor,
		 * but does not deallocate the memory.
		 */
		template <class T, class GC>
		struct CustomControlBlock : public BaseControlBlock<T, GC> {
			template <std::convertible_to<T> Resource>
			CustomControlBlock(Resource&& rc, GC const& collector = GC())
				: BaseControlBlock<T, GC>(this, std::forward<Resource>(rc), collector) {}

			void Release() noexcept {
				if (this->m_ref_count.fetch_sub(1, std::memory_order::acq_rel) == 1u) {
					this->~CustomControlBlock();   // Destroy the control block, but memory remains.
				}
			}
		};

	} // namespace details

	/**
	 * @brief A reference‑counted shared resource, similar to std::shared_ptr, but
	 *        with support for custom garbage collection and placement construction.
	 *
	 * The control block is polymorphic and can be either heap‑allocated (default)
	 * or placed in user‑supplied storage (via placement new). The garbage collector
	 * (GC) is invoked when the resource is finally destroyed.
	 *
	 * @tparam T The type of the managed resource.
	 * @tparam GC The garbage collector functor type (same as for UniqueResource).
	 */
	export template <class T, class GC = details::EmptyGC>
	class SharedResource final {
	private:
		using ControlBlock = details::BaseControlBlock<T, GC>;
		ControlBlock* m_block;   ///< Pointer to the control block (nullptr if empty).

	public:
		/// Constructs an empty SharedResource.
		SharedResource() : m_block(nullptr) {}

		/**
		 * @brief Constructs a SharedResource that owns a new resource, using a heap‑allocated
		 *        default control block.
		 * @tparam Resource A type convertible to T.
		 * @param rc The resource to manage.
		 * @param collector The garbage collector (default‑constructed if omitted).
		 */
		template <std::convertible_to<T> Resource>
		SharedResource(Resource&& rc, GC const& collector = GC())
			: m_block(new details::DefaultHeapControlBlock<T, GC>(std::forward<Resource>(rc), collector)) {}

		/// @overload
		SharedResource(T const& rc, GC const& collector = GC())
			: m_block(new details::DefaultHeapControlBlock<T, GC>(rc, collector)) {}

		/// @overload
		SharedResource(T&& rc, GC const& collector = GC())
			: m_block(new details::DefaultHeapControlBlock<T, GC>(std::move(rc), collector)) {}

		/**
		 * @brief Constructs a SharedResource with a user‑provided buffer for the control block.
		 *        The buffer must be large enough and properly aligned.
		 * @tparam Resource A type convertible to T.
		 * @param buffer Pointer to storage (must be at least RequiredBufferSize() bytes).
		 * @param rc The resource to manage.
		 * @param collector The garbage collector.
		 */
		template <std::convertible_to<T> Resource>
		SharedResource(void* buffer, Resource&& rc, GC const& collector = GC())
			: m_block(new(buffer) details::CustomControlBlock<T, GC>(std::forward<Resource>(rc), collector)) {}

		/// @overload
		SharedResource(void* buffer, T const& rc, GC const& collector = GC())
			: m_block(new(buffer) details::CustomControlBlock<T, GC>(rc, collector)) {}

		/// @overload
		SharedResource(void* buffer, T&& rc, GC const& collector = GC())
			: m_block(new(buffer) details::CustomControlBlock<T, GC>(std::move(rc), collector)) {}

		/// Returns the required buffer size for placement construction.
		static consteval std::size_t RequiredBufferSize() noexcept {
			return sizeof(ControlBlock);
		}

		/// Copy constructor – shares ownership with the other instance.
		SharedResource(SharedResource const& other) noexcept : m_block(other.m_block) {
			if (m_block) m_block->AddReference();
		}

		/// Move constructor – transfers ownership.
		SharedResource(SharedResource&& other) noexcept
			: m_block(std::exchange(other.m_block, nullptr)) {}

		/// Destructor – releases ownership.
		~SharedResource() noexcept {
			if (m_block) m_block->Release();
		}

		/// Copy assignment.
		SharedResource& operator=(SharedResource const& other) noexcept {
			if (this != &other) {
				if (m_block) m_block->Release();
				m_block = other.m_block;
				if (m_block) m_block->AddReference();
			}
			return *this;
		}

		/// Move assignment.
		SharedResource& operator=(SharedResource&& other) noexcept {
			if (this != &other) {
				if (m_block) m_block->Release();
				m_block = std::exchange(other.m_block, nullptr);
			}
			return *this;
		}

		/// Releases ownership of the current resource (if any). After this, the SharedResource is empty.
		void Reset() noexcept {
			if (m_block) {
				m_block->Release();
				m_block = nullptr;
			}
		}

		/// Returns the current reference count (useful for debugging).
		std::size_t UseCount() const noexcept {
			return m_block ? m_block->m_ref_count.load(std::memory_order::relaxed) : 0;
		}

		/// Boolean conversion – true if non‑empty.
		explicit operator bool() const noexcept { return m_block != nullptr; }

		/// Returns a reference to the garbage collector (const‑qualified according to self).
		template <class Self>
		auto GetGC(this Self&& self) noexcept
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, GC const&, GC&> {
			return self.m_block->m_impl.GetGC();
		}

		/// Returns a reference to the managed resource.
		template <class Self>
		auto Get(this Self&& self) noexcept
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, T const&, T&> {
			return self.m_block->m_impl.Get();
		}

		/// Arrow operator for non‑pointer T – returns pointer to the resource.
		template <class Self>
		auto operator->(this Self&& self) noexcept
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, T const*, T*>
			requires (!details::PointerTraits<T>::is_pointer) {
			return &self.Get();
		}

		/// Arrow operator for pointer T – returns the raw pointer itself.
		template <class Self>
		auto operator->(this Self&& self) noexcept
			-> std::conditional_t<
				std::is_const_v<std::remove_reference_t<Self>>,
				typename details::PointerTraits<T>::const_pointer,
				typename details::PointerTraits<T>::pointer>
			requires (details::PointerTraits<T>::is_pointer) {
			return self.Get();
		}

		/// Dereference operator – returns a reference to the resource.
		template <class Self>
		auto operator*(this Self&& self) noexcept
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, T const&, T&> {
			return self.Get();
		}

		/// Implicit conversion to T&.
		operator T& () noexcept { return *m_block->m_impl; }
		/// Implicit conversion to T const&.
		operator T const& () const noexcept { return *m_block->m_impl; }

		/**
		 * @brief Dereference operator for when T is a pointer – returns a reference to the pointee.
		 * Only enabled when T is a pointer.
		 */
		template <class Self, class U = T>
		auto operator*(this Self&& self) noexcept
			-> std::add_lvalue_reference_t<typename details::PointerTraits<U>::element_type>
			requires (details::PointerTraits<U>::is_pointer) {
			return *self.Get();
		}

		/// Defaulted three‑way comparison (compares the control block pointers).
		std::strong_ordering operator<=>(SharedResource const& other) const noexcept = default;
	};

} // namespace plastic::utility