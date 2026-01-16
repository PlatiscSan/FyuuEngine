export module core:basic_shape;
import std;
import math;

namespace fyuu_engine::core::resource {

	struct Box {
		math::Vector3 half_extents;
	};

	struct Sphere {
		float radius;
	};

	struct Capsule {
		float radius;
		float half_height;
	};

}