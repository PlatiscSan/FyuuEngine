export module math:eigen_matrix;
import std;
import <Eigen/Dense>;
import <Eigen/Geometry>;
import :base_matrix;

namespace fyuu_engine::math {

	template <std::size_t dim> class EigenMatrix : public BaseMatrix<EigenMatrix<dim>, dim> {
		friend class BaseMatrix<EigenMatrix<dim>, dim>;
	private:
		Eigen::Matrix<float, dim, dim> m_eigen_impl;

	public:
		EigenMatrix() = default;

		template <std::size_t N>
		EigenMatrix(Eigen::Matrix<float, N, N> const& eigen_matrix)
			: m_eigen_impl(eigen_matrix) {

		}

	};

	export using EigenMatrix2 = EigenMatrix<2>;
	export using EigenMatrix3 = EigenMatrix<3>;
	export using EigenMatrix4 = EigenMatrix<4>;

}