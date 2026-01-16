export module plastic.unique_resource;
export import plastic.disable_copy;
import std;

namespace plastic::utility {
	export template <class T, class GC> class UniqueResource final
		: public DisableCopy<UniqueResource<T, GC>> {
	private:
		std::optional<T> m_rc;
		GC m_gc;

	public:
		UniqueResource() noexcept = default;

		template <std::convertible_to<T> Resource>
		UniqueResource(Resource&& rc, GC const& collector = GC())
			: m_rc(std::in_place, std::forward<Resource>(rc)),
			m_gc(collector) {
		}

		UniqueResource(UniqueResource const&) = delete;
		UniqueResource& operator=(UniqueResource const&) = delete;

		UniqueResource(UniqueResource&& other) noexcept
			: m_rc(std::move(other.m_rc)),
			m_gc(std::move(other.m_gc)) {
			other.m_rc.reset();
		}

		UniqueResource& operator=(UniqueResource&& other) noexcept {
			if(this != &other) {
				if(m_rc) {
					T moved = std::move(*m_rc);
					m_gc(moved);
				}
				m_rc = std::move(other.m_rc);
				m_gc = std::move(other.m_gc);
				other.m_rc.reset();
			}
			return *this;
		}

		void Reset() noexcept {
			if(m_rc) {
				T moved = std::move(*m_rc);
				m_rc.reset();
				m_gc(moved);
			}
		}

		~UniqueResource() noexcept {
			Reset();
		}

		auto GetGC(this auto&& self) noexcept
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<decltype(self)>>, GC const&, GC&> {
			return self.m_gc;
		}

		auto Get(this auto&& self) noexcept 
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<decltype(self)>>, T const&, T&> {
			return *self.m_rc;
		}

		auto operator->(this auto&& self) noexcept
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<decltype(self)>>, T const*, T*> {
			return &(*self.m_rc);
		}

		auto operator*(this auto&& self) noexcept
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<decltype(self)>>, T const&, T&> {
			return self.Get();
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