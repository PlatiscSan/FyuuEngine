export module math;
import std;
import :eigen_vector;
import :eigen_matrix;
import :eigen_quaternion;

namespace fyuu_engine::math {

	export using Vector2 = EigenVector2;
	export using Vector3 = EigenVector3;
	export using Vector4 = EigenVector4;

	export using Matrix2 = EigenMatrix2;
	export using Matrix3 = EigenMatrix3;
	export using Matrix4 = EigenMatrix4;

	export using Quaternion = EigenQuaternion;


}