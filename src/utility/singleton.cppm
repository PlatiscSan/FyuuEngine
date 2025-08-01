export module singleton;
import std;
import disable_copy;
import defer;

export namespace util {

	template <class T>
	class Singleton : public DisableCopy<Singleton<T>> {
	private:
		static inline T* s_instance;
		static inline std::atomic_flag s_mutex;

		static auto SingletonLock() noexcept {
			bool mutex = false;
			do {
				mutex = s_mutex.test_and_set(std::memory_order::acq_rel);
				if (mutex) {
					s_mutex.wait(true, std::memory_order::relaxed);
				}
			} while (mutex);
			return Defer(
				[]() {
					s_mutex.clear(std::memory_order::release);
					s_mutex.notify_one();
				}
			);
		}

	public:
		static void DestroyInstance() {
			auto lock = Singleton::SingletonLock();
			if (s_instance) {
				delete s_instance;
				s_instance = nullptr;
			}
		}

		static T* Instance() {
			struct GC {
				~GC() noexcept {
					Singleton::DestroyInstance();
				}
			};
			
			auto lock = Singleton::SingletonLock();
			if (!s_instance) {
				static GC gc;
				s_instance = new T();
			}

			return s_instance;
		}

	};

}