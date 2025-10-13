export module singleton;
import std;
import disable_copy;
import defer;

export namespace fyuu_engine::util {

	template <class T>
	class Singleton : public DisableCopy<Singleton<T>> {
	private:
		static inline T* s_instance;
		static inline std::atomic_flag s_mutex;

	public:
		static void DestroyInstance() {
			auto lock = Lock(s_mutex);
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
			
			auto lock = Lock(s_mutex);
			if (!s_instance) {
				static GC gc;
				s_instance = new T();
			}

			return s_instance;
		}

	};

}