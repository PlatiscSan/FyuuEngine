export module plastic.singleton;
import std;
import plastic.disable_copy;

export namespace plastic::utility {

	template <class T>
	class Singleton : public DisableCopy<Singleton<T>> {
	private:
		static inline T* s_instance;
		static inline std::mutex s_mutex;

	public:
		static void DestroyInstance() {
			std::unique_lock<std::mutex> lock(s_mutex);
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
			
			std::unique_lock<std::mutex> lock(s_mutex);
			if (!s_instance) {
				static GC gc;
				s_instance = new T();
			}

			return s_instance;
		}

	};

}