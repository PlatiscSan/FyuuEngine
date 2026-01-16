export module plastic.other;
import std;

namespace plastic::utility {

	export template <class Func> class Defer final {
	private:
		Func m_func;

	public:
		Defer(Func&& func)
			: m_func(std::forward<Func>(func)) {
		}

		Defer(Defer const&) = delete;
		Defer& operator=(Defer const&) = delete;

		Defer(Defer&&) = default;
		Defer& operator=(Defer&&) = default;

		~Defer() noexcept {
			try { std::invoke(m_func); }
			catch (...) {}
		}
	};

	export template <class Func> void InitializeGlobalInstance(Func&& func) {

		static std::once_flag once_flag;
		static std::atomic_flag initialized;

		std::call_once(
			once_flag,
			[&func]() {
				func();
				initialized.test_and_set(std::memory_order::relaxed);
				initialized.notify_all();
			}
		);

		if (!initialized.test(std::memory_order::relaxed)) {
			initialized.wait(false, std::memory_order::relaxed);
		}
	}

	export extern std::vector<std::byte> ReadFile(std::filesystem::path const& path);

}