module math:eigen_quaternion;

namespace fyuu_engine::math {

	EigenMatrix3 EigenQuaternion::ToRotationMatrixImpl() const {
		return m_eigen_impl.toRotationMatrix();
	}

	EigenQuaternion::EigenQuaternion(float w, float x, float y, float z)
		: m_eigen_impl(w, x, y, z) {
		
	}
}