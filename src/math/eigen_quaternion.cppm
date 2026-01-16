export module math:eigen_quaternion;
import std;
import :base_quaternion;
import <Eigen/Dense>;
import <Eigen/Geometry>;
import :eigen_matrix;

namespace fyuu_engine::math {

	export class EigenQuaternion;

	namespace details {
		template <> 
		struct QuaternionTraits<EigenQuaternion> {
			using Matrix3 = EigenMatrix3;
		};
	}

	export class EigenQuaternion : public BaseQuaternion<EigenQuaternion> {
		friend class BaseQuaternion<EigenQuaternion>;
	private:
		Eigen::Quaternionf m_eigen_impl;

		EigenMatrix3 ToRotationMatrixImpl() const;

	public:
		EigenQuaternion(float w, float x, float y, float z);
	};

}