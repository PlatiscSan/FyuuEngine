// ============================================================================
// registrable.impl.cpp - Implementation of the registration helpers
// ============================================================================
//
// This file contains the definitions of the internal functions declared in
// the module interface. It maintains a global thread‑safe map from IDs to
// object pointers.

module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <unordered_map>
#include <shared_mutex>
#include <atomic> 
#include <stdexcept> 
#include <memory>
#endif // !defined(__cpp_lib_modules)

module plastic.registrable:impl;

#if defined(__cpp_lib_modules)
import std; 
#endif // defined(__cpp_lib_modules)

namespace plastic::utility::details {

	// ------------------------------------------------------------------------
	// Internal registry entry – simply holds a pointer to the object.
	// (Could be extended later, e.g., with type information.)
	// ------------------------------------------------------------------------
	struct Entry {
		void* ptr;
	};

	// Global registry: maps a unique ID to the corresponding object pointer.
	static std::unordered_map<std::size_t, Entry> s_registry;

	// Mutex protecting concurrent access to the registry.
	static std::shared_mutex s_registry_mutex;

	// Atomic counter for generating unique IDs.
	static std::atomic_size_t s_id_counter{ 0 };

	// ------------------------------------------------------------------------
	// Registers a new object with a given ID. Throws if the ID already exists.
	// ------------------------------------------------------------------------
	void RegisterObject(std::size_t id, void* obj) {
		std::lock_guard<std::shared_mutex> lock(s_registry_mutex);
		auto [it, inserted] = s_registry.try_emplace(id, Entry{ obj });
		if (!inserted) {
			throw std::runtime_error("Duplicate ID registration");
		}
	}

	// ------------------------------------------------------------------------
	// Updates the pointer associated with an existing ID (used after moving
	// an object). Throws if the ID does not exist.
	// ------------------------------------------------------------------------
	void UpdateRegistration(std::size_t id, void* obj) {
		std::lock_guard<std::shared_mutex> lock(s_registry_mutex);
		auto it = s_registry.find(id);
		if (it == s_registry.end()) {
			throw std::runtime_error("No ID registration");
		}
		it->second.ptr = obj;
	}

	// ------------------------------------------------------------------------
	// Unregisters an object. The operation succeeds only if the stored pointer
	// matches the provided one (prevents accidental erasure of a different
	// object that happens to have the same ID due to some bug).
	// ------------------------------------------------------------------------
	void UnregisterObject(std::size_t id, void* obj) noexcept {
		std::lock_guard<std::shared_mutex> lock(s_registry_mutex);
		auto it = s_registry.find(id);
		if (it != s_registry.end() && it->second.ptr == obj) {
			s_registry.erase(it);
		}
		// If the pointer does not match, we silently do nothing – this indicates
		// a logic error, but we choose not to throw in a destructor.
	}

	// ------------------------------------------------------------------------
	// Generates a new unique ID (monotonically increasing).
	// ------------------------------------------------------------------------
	std::size_t IDGen() noexcept {
		return s_id_counter.fetch_add(1u, std::memory_order_relaxed);
	}

	// ------------------------------------------------------------------------
	// Internal lookup (used by QueryObject). Returns the pointer associated
	// with the ID, or nullptr if the ID is not found.
	// ------------------------------------------------------------------------
	static void* LookupObject(std::size_t id) noexcept {
		std::shared_lock<std::shared_mutex> lock(s_registry_mutex);
		auto it = s_registry.find(id);
		return (it != s_registry.end()) ? it->second.ptr : nullptr;
	}

	// ------------------------------------------------------------------------
	// Public query function – simply forwards to LookupObject.
	// ------------------------------------------------------------------------
	void* QueryObject(std::size_t id) noexcept {
		return LookupObject(id);
	}

} // namespace plastic::utility::details