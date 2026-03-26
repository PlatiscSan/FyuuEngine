// ============================================================================
// other.cppm - Module interface for miscellaneous utilities
// ============================================================================
//
// This module provides two small but useful utilities:
//   - Defer<Func> : a RAII class that executes a function upon destruction
//                   (similar to Go's `defer` statement).
//   - InitializeGlobalInstance<Func> : a thread‑safe function that ensures a
//                                      global initializer is run exactly once
//                                      and that all callers wait until it
//                                      completes.

module; // Global module fragment – includes necessary headers when modules are not supported

#if !defined(__cpp_lib_modules)
#include <version>
// Include standard headers required by the implementation.
#include <functional>   // std::invoke
#include <mutex>        // std::once_flag, std::call_once
#include <atomic>       // std::atomic_flag, test_and_set, notify_all, wait
#include <utility>      // std::move
#endif // !defined(__cpp_lib_modules)

export module plastic.other;

#if defined(__cpp_lib_modules)
import std;              // Import the entire standard library if module support is available.
#endif // defined(__cpp_lib_modules)

namespace plastic::utility {

	// ------------------------------------------------------------------------
	// Defer – execute a function on scope exit
	// ------------------------------------------------------------------------

	/**
	 * @brief A RAII wrapper that invokes a supplied function when the Defer
	 *        object goes out of scope. It is move‑only and cannot be copied.
	 *
	 * Typical use:
	 * @code
	 *   auto d = Defer([]{ cleanup(); });
	 * @endcode
	 *
	 * @tparam Func The type of the function object to invoke. It must be
	 *              callable without arguments and should not throw exceptions
	 *              (though exceptions are caught and ignored in the destructor).
	 */
	export template <class Func>
	class Defer final {
	private:
		Func m_func;   ///< Stored function object.

	public:
		/**
		 * @brief Constructs a Defer from a function object.
		 * @param func The function to be called on destruction.
		 */
		Defer(Func&& func) : m_func(std::move(func)) {}

		// Copy is disallowed – a deferred action should not be duplicated.
		Defer(Defer const&) = delete;
		Defer& operator=(Defer const&) = delete;

		// Move is allowed (transfers ownership of the function).
		Defer(Defer&&) = default;
		Defer& operator=(Defer&&) = default;

		/// Destructor invokes the stored function. Any exception is swallowed.
		~Defer() noexcept {
			try {
				std::invoke(m_func);
			} catch (...) {
				// According to the design, the function should not throw,
				// but if it does, we ignore it to satisfy noexcept.
			}
		}
	};

	// ------------------------------------------------------------------------
	// InitializeGlobalInstance – thread‑safe one‑time initializer with waiting
	// ------------------------------------------------------------------------

	/**
	 * @brief Ensures that a given initializer function is executed exactly once,
	 *        and that all callers wait until it completes before proceeding.
	 *
	 * This is useful for lazy global initialization where multiple threads may
	 * race to initialize a global resource. The first caller runs the initializer;
	 * subsequent callers block until the initializer finishes.
	 *
	 * @tparam Func A callable type (invocable with no arguments).
	 * @param func The initializer function to run once.
	 *
	 * @note The implementation uses std::call_once to guarantee single execution,
	 *       and an atomic_flag with waiting to block later callers until the
	 *       initializer is done.
	 */
	export template <class Func>
	void InitializeGlobalInstance(Func&& func) {
		static std::once_flag once_flag;
		static std::atomic_flag initialized;   // Initially in clear state (false)

		// Call the initializer exactly once.
		std::call_once(
			once_flag,
			[&func]() {
				func();                               // run the user function
				initialized.test_and_set(std::memory_order::relaxed); // set flag to true
				initialized.notify_all();              // wake up any waiters
			}
		);

		// If the initializer hasn't finished yet (rare race where this thread
		// is not the one that executed call_once but hasn't seen the flag yet),
		// wait until it is set.
		if (!initialized.test(std::memory_order::relaxed)) {
			initialized.wait(false, std::memory_order::relaxed);
		}
	}

} // namespace plastic::utility