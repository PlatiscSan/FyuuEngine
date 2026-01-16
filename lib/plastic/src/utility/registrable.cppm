export module plastic.registrable;
import std;
import plastic.any_pointer;

namespace plastic::utility {

	namespace details {

		extern std::size_t IDGen() noexcept;
		extern void RegisterObject(std::size_t id, void* obj);
		extern void UnregisterObject(std::size_t id, void* obj) noexcept;
		extern void* QueryObject(std::size_t id);

	}

	export template <class T> auto QueryObject(std::size_t id) 
		-> plastic::utility::AnyPointer<T> {
		if constexpr (std::derived_from<T, EnableSharedFromThis<T>>) {
			return static_cast<T*>(details::QueryObject(id))->This();
		}
		else {
			return static_cast<T*>(details::QueryObject(id));
		}
	}

	/// @brief	Make one class derive from Registrable to use global instance query, 
	///			whose first base class should be Registrable<Derived> if the auto
	///			registration is desired, otherwise the Register() must be called by
	///			the derived constructor
	/// @tparam T 
	export template <class T> class Registrable {
	private:
		std::once_flag m_registration_once;
		std::size_t m_id;

	protected:
		void UpdateRegistration(Registrable&& other) noexcept {
			details::UnregisterObject(m_id, this);
			details::RegisterObject(other.m_id, this);
		}

		struct NoBaseRegistration {};

		void Register() {
			std::call_once(
				m_registration_once,
				[this]() {
					m_id = details::IDGen();
					details::RegisterObject(m_id, this);
				}
			);
		}

	public:
		Registrable(NoBaseRegistration)
			: m_registration_once(),
			m_id(0u) {

		}

		Registrable()
			: m_registration_once(),
			m_id(details::IDGen()) {
			std::call_once(
				m_registration_once,
				[this]() {
					details::RegisterObject(m_id, this);
				}
			);
		}

		Registrable(Registrable const& other)
			: m_registration_once(),
			m_id(details::IDGen()) {
			std::call_once(
				m_registration_once,
				[this]() {
					details::RegisterObject(m_id, this);
				}
			);
		}

		Registrable(Registrable&& other) noexcept
			: m_registration_once(),
			m_id(std::exchange(other.m_id, 0ull)) {
			std::call_once(
				m_registration_once,
				[this]() {
					details::RegisterObject(m_id, this);
				}
			);
		}

		~Registrable() noexcept {
			details::UnregisterObject(m_id, this);
		}

		std::size_t GetID() const noexcept {
			return m_id;
		}

	};


}