export module synchronized_function;
import std;
import defer;
export namespace concurrency {

	template <class>
	class SynchronizedFunction;

	template <class R, class... Args >
	class SynchronizedFunction<R(Args...)> {
	public:
		using Func = std::function<R(Args...)>;

		template <class GC>
		class Bool {
		private:
			GC m_gc;
			bool m_value;

		public:
			Bool(GC&& gc, bool value)
				:m_gc(std::move(gc)), m_value(value) {

			}

			explicit operator bool() const noexcept {
				return m_value;
			}
		};

	private:
		template <class GC>
		struct Locks {
			GC lock_a;
			GC lock_b;
			Locks(GC&& lock_a, GC&& lock_b) noexcept
				: lock_a(std::move(lock_a)), lock_b(std::move(lock_b)) {

			}
		};

		Func m_imp;
		mutable std::atomic_flag m_mutex = {};

		static auto Lock(std::atomic_flag& mutex) noexcept {
			bool loaded_mutex;
			do {
				loaded_mutex = mutex.test_and_set(std::memory_order::acq_rel);
				if (loaded_mutex) {
					mutex.wait(true, std::memory_order::relaxed);
				}
			} while (loaded_mutex);

			return util::Defer(
				[&mutex]() {
					mutex.clear(std::memory_order::release);
					mutex.notify_all();
				}
			);
		}

		static auto LockBoth(SynchronizedFunction const& a, SynchronizedFunction const& b) {

			if (&a < &b) {
				return Locks(SynchronizedFunction::Lock(a.m_mutex), SynchronizedFunction::Lock(b.m_mutex));
			}
			else {
				return Locks(SynchronizedFunction::Lock(b.m_mutex), SynchronizedFunction::Lock(a.m_mutex));
			}
		}

		template <class GC>
		struct LockedReference {
			Func& func;
			GC lock;

			LockedReference(Func& func, GC&& lock)
				: func(func), lock(std::move(lock)) {

			}

			Func& Get() const noexcept {
				return func;
			}

		};


	public:
		SynchronizedFunction() noexcept = default;
		SynchronizedFunction(std::nullptr_t) noexcept {

		}

		SynchronizedFunction(SynchronizedFunction const& other) 
			: m_imp(LockedReference(other.m_imp, Lock(other.m_mutex)).Get()) {
		}

		SynchronizedFunction(SynchronizedFunction&& other) noexcept 
			: m_imp(std::move(LockedReference(other.m_imp, Lock(other.m_mutex)).Get())) {

		}

		template <std::convertible_to<Func> Compatible>
		SynchronizedFunction(Compatible&& func)
			:m_imp(std::forward<Compatible>(func)) {
		}

		~SynchronizedFunction() noexcept = default;

		SynchronizedFunction& operator=(SynchronizedFunction const& other) {

			if (this != &other) {
				auto locks = SynchronizedFunction::LockBoth(*this, other);
				m_imp = other.m_imp;
			}

			return *this;

		}

		SynchronizedFunction& operator=(SynchronizedFunction&& other) {

			if (this != &other) {
				auto locks = SynchronizedFunction::LockBoth(*this, other);
				m_imp = std::move(other.m_imp);
			}

			return *this;

		}

		SynchronizedFunction& operator=(std::nullptr_t) noexcept {
			auto this_lock = Lock(m_mutex);
			m_imp = std::nullptr_t();
			return *this;
		}

		template <std::convertible_to<Func> Compatible>
		SynchronizedFunction& operator=(Compatible&& func) {
			m_imp = std::forward<Compatible>(func);
			return *this;
		}

		template <std::convertible_to<Func> Compatible>
		SynchronizedFunction& operator=(std::reference_wrapper<Compatible> func) noexcept {
			m_imp = std::forward<std::reference_wrapper<Compatible>>(func);
			return *this;
		}

		auto IsCallable() const noexcept {
			return Bool(Lock(m_mutex), static_cast<bool>(m_imp));
		}

		R operator()(Args... args) const {
			auto lock = Lock(m_mutex);
			return m_imp(std::forward<Args>(args)...);
		}

		R UnsafeCall(Args... args) const {
			return m_imp(std::forward<Args>(args)...);
		}

	};


}