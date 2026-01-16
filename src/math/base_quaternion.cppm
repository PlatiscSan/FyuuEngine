export module math:base_quaternion;
import std;

namespace fyuu_engine::math {

	namespace details {
		template <class Quaternion> 
		struct QuaternionTraits;
	}
	
	export template <class Derived> class BaseQuaternion {
	public:
		using Matrix3 = typename details::QuaternionTraits<Derived>::Matrix3;

		Matrix3 ToRotationMatrix() const {
			return static_cast<Derived*>(this)->ToRotationMatrixImpl();
		}

	};

}