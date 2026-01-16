export module core:camera_config;
import std;
export import math;
import config;
import reflective;
import serialization;

namespace fyuu_engine::core::resource {
	
	export struct CameraPose {
		math::Vector3 position;
		math::Vector3 target;
		math::Vector3 up;
	};
	
	export using ReflectiveCameraPose = decltype(
		reflection::MakeReflective(
			std::declval<CameraPose&>(),
			reflection::FieldDescriptor<"position", &CameraPose::position>(),
			reflection::FieldDescriptor<"target", &CameraPose::target>(),
			reflection::FieldDescriptor<"up", &CameraPose::up>()
		)
		);

	export struct CameraConfig {
		CameraPose pose;
		math::Vector2 aspect;
		float z_far;
		float z_near;
	};

	export using ReflectiveCameraConfig = decltype(
		reflection::MakeReflective(
			std::declval<CameraConfig&>(),
			reflection::FieldDescriptor<"pose", &CameraConfig::pose>(),
			reflection::FieldDescriptor<"aspect", &CameraConfig::aspect>(),
			reflection::FieldDescriptor<"z_far", &CameraConfig::z_far>(),
			reflection::FieldDescriptor<"z_near", &CameraConfig::z_near>()
		)
		);
}