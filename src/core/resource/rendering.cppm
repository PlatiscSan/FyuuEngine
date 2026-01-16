export module core:rendering;
import std;
import math;
import :camera_config;
import config;
import reflective;
import serialization;

namespace fyuu_engine::core::resource {
	
	export struct SkyBoxIrradianceMap {
		std::string m_negative_x;
		std::string m_positive_x;
		std::string m_negative_y;
		std::string m_positive_y;
		std::string m_negative_z;
		std::string m_positive_z;
	};

	export using ReflectiveSkyBoxIrradianceMap = decltype(
		reflection::MakeReflective(
			std::declval<SkyBoxIrradianceMap&>(),
			reflection::FieldDescriptor<"negative_x", &SkyBoxIrradianceMap::m_negative_x>(),
			reflection::FieldDescriptor<"positive_x", &SkyBoxIrradianceMap::m_positive_x>(),
			reflection::FieldDescriptor<"negative_y", &SkyBoxIrradianceMap::m_negative_y>(),
			reflection::FieldDescriptor<"positive_y", &SkyBoxIrradianceMap::m_positive_y>(),
			reflection::FieldDescriptor<"negative_z", &SkyBoxIrradianceMap::m_negative_z>(),
			reflection::FieldDescriptor<"positive_z", &SkyBoxIrradianceMap::m_positive_z>()
		)
		);

	export struct SkyBoxSpecularMap {
		std::string m_negative_x;
		std::string m_positive_x;
		std::string m_negative_y;
		std::string m_positive_y;
		std::string m_negative_z;
		std::string m_positive_z;
	};

	export using ReflectiveSkyBoxSpecularMap = decltype(
		reflection::MakeReflective(
			std::declval<SkyBoxSpecularMap&>(),
			reflection::FieldDescriptor<"negative_x", &SkyBoxSpecularMap::m_negative_x>(),
			reflection::FieldDescriptor<"positive_x", &SkyBoxSpecularMap::m_positive_x>(),
			reflection::FieldDescriptor<"negative_y", &SkyBoxSpecularMap::m_negative_y>(),
			reflection::FieldDescriptor<"positive_y", &SkyBoxSpecularMap::m_positive_y>(),
			reflection::FieldDescriptor<"negative_z", &SkyBoxSpecularMap::m_negative_z>(),
			reflection::FieldDescriptor<"positive_z", &SkyBoxSpecularMap::m_positive_z>()
		)
		);

	export struct DirectionalLight {
		math::Vector3 direction;
		math::Vector3 color;
	};

	export using ReflectiveDirectionalLight = decltype(
		reflection::MakeReflective(
			std::declval<DirectionalLight&>(),
			reflection::FieldDescriptor<"direction", &DirectionalLight::direction>(),
			reflection::FieldDescriptor<"color", &DirectionalLight::color>()
		)
		);

	export struct RenderingSettings {
		SkyBoxIrradianceMap skybox_irradiance_map;
		SkyBoxSpecularMap skybox_specular_map;
		std::string brdf_map;
		std::string color_grading_map;

		math::Vector3 sky_color;
		math::Vector3 ambient_light;
		CameraConfig camera_config;
		DirectionalLight directional_light;

		bool enable_fxaa;
	};

	export using ReflectiveRenderingSettings = decltype(
		reflection::MakeReflective(
			std::declval<RenderingSettings&>(),
			reflection::FieldDescriptor<"skybox_irradiance_map", &RenderingSettings::skybox_irradiance_map>(),
			reflection::FieldDescriptor<"skybox_specular_map", &RenderingSettings::skybox_specular_map>(),
			reflection::FieldDescriptor<"brdf_map", &RenderingSettings::brdf_map>(),
			reflection::FieldDescriptor<"color_grading_map", &RenderingSettings::color_grading_map>(),
			reflection::FieldDescriptor<"sky_color", &RenderingSettings::sky_color>(),
			reflection::FieldDescriptor<"ambient_light", &RenderingSettings::ambient_light>(),
			reflection::FieldDescriptor<"camera_config", &RenderingSettings::camera_config>(),
			reflection::FieldDescriptor<"directional_light", &RenderingSettings::directional_light>(),
			reflection::FieldDescriptor<"enable_fxaa", &RenderingSettings::enable_fxaa>()
		)
		);

	export class RenderingSettingsSerializer
		: public reflection::BaseJSONSerializer<RenderingSettingsSerializer> {
	public:
		using Base = reflection::BaseJSONSerializer<RenderingSettingsSerializer>;
		using Base::Deserialize;

		static ReflectiveRenderingSettings MakeReflective(RenderingSettings& data) {
			return ReflectiveRenderingSettings(data);
		}

		static ReflectiveSkyBoxIrradianceMap MakeReflective(SkyBoxIrradianceMap& data) {
			return ReflectiveSkyBoxIrradianceMap(data);
		}

		static ReflectiveSkyBoxSpecularMap MakeReflective(SkyBoxSpecularMap& data) {
			return ReflectiveSkyBoxSpecularMap(data);
		}

		static ReflectiveDirectionalLight MakeReflective(DirectionalLight& data) {
			return ReflectiveDirectionalLight(data);
		}

		static ReflectiveCameraConfig MakeReflective(CameraConfig& data) {
			return ReflectiveCameraConfig(data);
		}

		static ReflectiveCameraPose MakeReflective(CameraPose& data) {
			return ReflectiveCameraPose(data);
		}

		static void Deserialize(math::Vector3& vector, config::NodeValue const& node_val) {

			config::Array const& array = node_val.AsArray();

			config::Arithmetic const& component_x = array[0].AsArithmetic();
			config::Arithmetic const& component_y = array[1].AsArithmetic();
			config::Arithmetic const& component_z = array[2].AsArithmetic();

			vector[0] = component_x.As<float>();
			vector[1] = component_x.As<float>();
			vector[2] = component_x.As<float>();

		}

		static void Deserialize(math::Vector2& vector, config::NodeValue const& node_val) {

			config::Array const& array = node_val.AsArray();

			config::Arithmetic const& component_x = array[0].AsArithmetic();
			config::Arithmetic const& component_y = array[1].AsArithmetic();

			vector[0] = component_x.As<float>();
			vector[1] = component_x.As<float>();

		}

	};

}