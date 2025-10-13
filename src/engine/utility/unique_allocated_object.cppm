export module unique_allocated_object;
import std;
import disable_copy;
export namespace fyuu_engine::util {

	template <class T, class Allocator>
	class UniqueAllocatedObject : public DisableCopy<UniqueAllocatedObject<T, Allocator>> {
	private:
		std::size_t m_index;
		Allocator m_allocator;

	public:
		UniqueAllocatedObject(std::size_t index, Allocator const& allocator)
			: m_index(index),
			m_allocator(allocator) {

		}

		~UniqueAllocatedObject() {
			m_allocator.Release(m_index);
		}

		T* operator->() const noexcept {
			return &m_allocator.At(m_index);
		}

		std::add_lvalue_reference_t<T> operator*() const {
			return m_allocator.At(m_index);
		}

	};

}