// ============================================================================
// registrable.cppm - Module interface for object registration facility
// ============================================================================
//
// This module provides a CRTP base class `Registrable` that automatically
// assigns a unique ID to each instance of a derived type and registers it in
// a global registry. Objects can later be retrieved by ID using `QueryObject`.
// The registry is thread‑safe and supports move semantics.

module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstddef>      // std::size_t, std::ptrdiff_t
#include <cstdint>      // std::intptr_t
#include <optional>     // std::optional
#include <type_traits>  // std::is_polymorphic_v
#endif // !defined(__cpp_lib_modules)
export module plastic.registrable;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import plastic.disable_copy;

namespace plastic::utility {

	// ------------------------------------------------------------------------
	// Forward declarations of internal helper functions (implemented in the
	// module implementation file).
	// ------------------------------------------------------------------------
	namespace details {
		void RegisterObject(std::size_t id, void* obj);
		void UpdateRegistration(std::size_t id, void* obj);
		void UnregisterObject(std::size_t id, void* obj) noexcept;
		std::size_t IDGen() noexcept;
		void* QueryObject(std::size_t id) noexcept;
	}

	// ------------------------------------------------------------------------
	// CRTP base class that gives each derived object a unique ID and registers
	// it in a global map. The derived type must be non‑polymorphic.
	//
	// @tparam Derived The type that inherits from Registrable<Derived>.
	// ------------------------------------------------------------------------
	export template <class Derived> class Registrable {
	private:
		// Offset from the Registrable subobject to the complete Derived object.
		// This is needed to recover the Derived pointer from `this` when
		// registering or unregistering.
		std::ptrdiff_t const m_offset;

		// Optional ID assigned to this object. It becomes empty after a move.
		std::optional<std::size_t> m_id;

	protected:
		// --------------------------------------------------------------------
		// Constructor used by the derived class (passed a pointer to itself).
		// Computes the offset and generates a new unique ID, then registers
		// the complete Derived object.
		// --------------------------------------------------------------------
		Registrable(Derived* derived) noexcept
			: m_offset(reinterpret_cast<std::intptr_t>(this) - reinterpret_cast<std::intptr_t>(derived)),
			  m_id(details::IDGen()) {
			details::RegisterObject(*m_id, derived);
		}

		// --------------------------------------------------------------------
		// Copy constructor: the new object gets its own fresh ID and is
		// registered separately. The offset is copied from the source because
		// the relative position of the Registrable subobject within Derived is
		// the same for all instances.
		// --------------------------------------------------------------------
		Registrable(Registrable const& other) noexcept
			: m_offset(other.m_offset),
			  m_id(details::IDGen()) {
			// Recover the complete Derived object pointer by subtracting the
			// stored offset from `this`.
			void* derived = reinterpret_cast<void*>(reinterpret_cast<std::intptr_t>(this) - m_offset);
			details::RegisterObject(*m_id, derived);
		}

		// --------------------------------------------------------------------
		// Move constructor: transfers the ID from the source and updates the
		// registry entry to point to the new location of the object. The
		// source is left without an ID (moved-from state).
		// --------------------------------------------------------------------
		Registrable(Registrable&& other) noexcept
			: m_offset(other.m_offset),
			  m_id(std::move(other.m_id)) {
			void* derived = reinterpret_cast<void*>(reinterpret_cast<std::intptr_t>(this) - m_offset);
			details::UpdateRegistration(*m_id, derived);
		}

	public:
		// --------------------------------------------------------------------
		// Destructor: unregisters this object from the global registry, but
		// only if it still owns an ID (i.e., not moved‑from).
		// --------------------------------------------------------------------
		~Registrable() noexcept {
			static_assert(!std::is_polymorphic_v<Derived>,
				"Derived must not be polymorphic");
			if (m_id) {
				void* derived = reinterpret_cast<void*>(
					reinterpret_cast<std::intptr_t>(this) - m_offset);
				details::UnregisterObject(*m_id, derived);
			}
		}

		// --------------------------------------------------------------------
		// Returns the unique ID assigned to this object, or std::nullopt if
		// the object has been moved from.
		// --------------------------------------------------------------------
		std::optional<std::size_t> GetID() const noexcept {
			return m_id;
		}

		// Copy and move assignment are implicitly deleted because the class
		// contains a const member (m_offset) and a non‑copyable optional.
		// This is intentional – registrable objects should not be assigned.
	};

	// ------------------------------------------------------------------------
	// Global lookup function: given an ID, returns a pointer to the registered
	// object of type Queryable, or nullptr if no object with that ID exists
	// or the type does not match.
	//
	// @tparam Queryable The type to cast the found object to (must be
	//                   non‑polymorphic, but this is a safety check).
	// @param id The unique ID previously obtained from a Registrable object.
	// @return Pointer to the object, or nullptr.
	// ------------------------------------------------------------------------
	export template <class Queryable>
	Queryable* QueryObject(std::size_t id) noexcept {
		static_assert(!std::is_polymorphic_v<Queryable>,
			"Queryable type must not be polymorphic");
		void* ptr = details::QueryObject(id);
		return static_cast<Queryable*>(ptr);
	}

} // namespace plastic::utility