export module math:eigen_vector;
import <Eigen/Dense>;
import <Eigen/Geometry>;
import :base_vector;

namespace fyuu_engine::math {

	template <std::size_t dim> class EigenVector : public BaseVector<EigenVector<dim>, dim> {
		friend class BaseVector<EigenVector<dim>, dim>;
	private:
		Eigen::Vector<float, dim> m_eigen_impl;

	public:
		EigenVector() = default;

		EigenVector(Eigen::Vector<float, dim> const& eigen_vec)
			: m_eigen_impl(eigen_vec) {
		}

		float At(size_t index) const {
			return m_eigen_impl[index];
		}

		float& At(size_t index) {
			return m_eigen_impl[index];
		}

		bool IsEqualTo(EigenVector const& other) const {
			return m_eigen_impl == other.m_eigen_impl;
		}

		EigenVector Add(EigenVector const& other) const {
			return EigenVector(m_eigen_impl + other.m_eigen_impl);
		}

		EigenVector Sub(EigenVector const& other) const {
			return EigenVector(m_eigen_impl - other.m_eigen_impl);
		}

		EigenVector Scale(float scalar) const {
			return EigenVector(m_eigen_impl * scalar);
		}

		EigenVector& AddedBy(EigenVector const& other) {
			m_eigen_impl += other.m_eigen_impl;
			return *this;
		}

		EigenVector& SubtractedFrom(EigenVector const& other) {
			m_eigen_impl -= other.m_eigen_impl;
			return *this;
		}
	};

	export using EigenVector2 = EigenVector<2>;
	export using EigenVector3 = EigenVector<3>;
	export using EigenVector4 = EigenVector<4>;


}