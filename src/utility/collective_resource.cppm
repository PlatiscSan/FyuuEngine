export module collective_resource;
import std;
namespace util {
	export template <class T, class Collector>
	class UniqueCollectiveResource final {
	private:
		std::optional<T> m_rc;
		Collector m_collector;

	public:
		constexpr UniqueCollectiveResource() noexcept
			: m_rc(),
			m_collector() {

		}

		constexpr UniqueCollectiveResource(std::nullptr_t) noexcept
			: m_rc(),
			m_collector() {

		}

		constexpr UniqueCollectiveResource(std::nullopt_t) noexcept
			: m_rc(),
			m_collector() {

		}

		template <std::convertible_to<T> CompatibleResource>
		UniqueCollectiveResource(CompatibleResource&& rc, Collector const& collector = Collector())
			: m_rc(std::in_place, std::forward<CompatibleResource>(rc)),
			m_collector(collector) {
		}

		UniqueCollectiveResource(UniqueCollectiveResource const&) = delete;
		UniqueCollectiveResource& operator=(UniqueCollectiveResource const&) = delete;

		UniqueCollectiveResource(UniqueCollectiveResource&& other) noexcept
			: m_rc(std::move(other.m_rc)),
			m_collector(std::move(other.m_collector)) {
			other.m_rc.reset();
		}

		UniqueCollectiveResource& operator=(UniqueCollectiveResource&& other) noexcept {
			if(this != &other) {
				if(m_rc) {
					m_collector(std::move(*m_rc));
				}
				m_rc = std::move(other.m_rc);
				m_collector = std::move(other.m_collector);
				other.m_rc.reset();
			}
			return *this;
		}

		~UniqueCollectiveResource() noexcept {
			if(m_rc) {
				m_collector(std::move(*m_rc));
			}
		}

		void Reset() noexcept {
			if(m_rc) {
				m_collector(std::move(*m_rc));
				m_rc.reset();
			}
		}

		T& Get() noexcept {
			return *m_rc;
		}

		T const& Get() const noexcept {
			return *m_rc;
		}

		operator T&() noexcept {
			return *m_rc;
		}

		operator T const&() const noexcept {
			return *m_rc;
		}

		operator bool() const noexcept {
			return m_rc.has_value();
		}

	};
}