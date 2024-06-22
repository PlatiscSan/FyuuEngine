#ifndef SINGLETON_H
#define SINGLETON_H

import std;

namespace Fyuu {

	template <class T> class Singleton {

	public:

		static T* GetInstance() {

			std::lock_guard<std::mutex> lock(m_singleton_mutex);
			if (!m_instance) {
				try {
					m_instance.reset(new T());
				}
				catch (std::exception const&) {
					throw;
				}
			}
			return m_instance.get();
		}

		template <class... Arguments>
		static T* GetInstance(Arguments&&...args) {

			std::lock_guard<std::mutex> lock(m_singleton_mutex);
			if (!m_instance) {
				try {
					m_instance.reset(new T(std::forward<Arguments>(args)...));
				}
				catch (std::exception const&) {
					throw;
				}
			}
			return m_instance.get();
		}

		static void DestroyInstance() {

			std::lock_guard<std::mutex> lock(m_singleton_mutex);
			m_instance.reset();

		}

	private:

		static std::unique_ptr<T> m_instance;
		static std::mutex m_singleton_mutex;

	};


	template <class T> std::unique_ptr<T> Singleton<T>::m_instance = nullptr;
	template <class T> std::mutex Singleton<T>::m_singleton_mutex{};


}

#endif // !SINGLETON_H
