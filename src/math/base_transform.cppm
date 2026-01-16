export module math:base_transform;
import std;

namespace fyuu_engine::math {

	namespace details {
		template <class Quaternion>
		struct TransformTraits;
	}

	export template <class Derived> struct BaseTransform {
		using Vector3 = typename details::TransformTraits<Derived>::Vector3;
	};

}