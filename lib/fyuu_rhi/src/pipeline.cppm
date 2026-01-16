export module fyuu_rhi:pipeline;
import std;
import plastic.disable_copy;

namespace fyuu_rhi {

	export enum class ShaderStage : std::uint8_t {
		Unknown,
		Vertex,
		Fragment,
		Pixel = Fragment,
		Compute,
		Geometry,
		TessellationControl,
		TessellationEvaluation,
		Amplification,
		Mesh,
		RayGeneration,
		RayIntersection,
		RayAnyHit,
		RayClosestHit,
		RayMiss,
		Callable,
		Count
	};

	export enum class ShaderLanguage : std::uint8_t {
		Unknown,
		HLSL,
		GLSL,
		MSL,       // Metal Shading Language
		WGSL       // WebGPU
	};

	export enum class OptimizationLevel : std::uint8_t {
		Unknown,
		O1,        // -O1
		O2,        // -O2
		O3,        // -O3
		Size       // -Os
	};

	export struct ShaderMacro {
		std::string_view name;
		std::string_view value = "1";
	};

	export struct ShaderLibraryReference {
		std::string name;
		std::filesystem::path path;
		std::string prefix;
		bool is_public = false;
	};

	export struct IncludeDirectory {
		std::filesystem::path path;
		bool is_system = false;
		bool recursive = false;
	};

	enum HLSLOptions : std::uint8_t {
		HLSLOptionsEnable16BitType,
		HLSLOptionsCount
	};

	export struct CompilationOptions {
		
		std::string entry_point = "main";

		/// @brief	the variable will be used in glslang::TShader::setPreamble if the shader language is GLSL,
		///			where the macro will added like #define MACRO 1 before compiling to SPIR-V.
		///			When the language is HLSL, it will be added as parameter like dxc -T MACRO=1 
		std::span<ShaderMacro> macros;

		std::bitset<static_cast<std::size_t>(HLSLOptions::HLSLOptionsCount)> hlsl_options;

#if defined(_WIN32)
		OptimizationLevel dxc_optimization = OptimizationLevel::O3;
#endif // defined(_WIN32)


	};

	export struct ShaderResourceBinding {
		std::string name;
		std::uint32_t bind_point;
		std::uint32_t bind_count;
		//std::bitset<static_cast<std::size_t>(ShaderStage::Count)> visibility;
		bool is_writable;
		enum class Type : std::uint8_t {
			ConstantBuffer,
			Texture,
			Sampler,
			UAV,
			StructuredBuffer,
			ByteAddressBuffer
		} type;
	};

	export struct ShaderReflection {
		std::vector<ShaderResourceBinding> resources;
		std::string entry_point;
		//ShaderStage stage;

		std::unordered_map<std::string, uint32_t> bindings;

	};

	export class IShaderLibrary 
		: public plastic::utility::DisableCopy<IShaderLibrary> {
	public:

	};

	export class BaseRenderingPipeline {
	public:

	};

	enum class StageVisibility : uint8_t {
		Unknown = 0,
		Vertex = 1,
		Fragment = 1 << 1,
		Compute = 1 << 2,
	};

	enum class BindingType : uint8_t {
		Unknown = 0,
		Sampler, 
		SampledImage, 
		StorageImage, 
		UniformTexelBuffer, 
		StorageTexelBuffer, 
		UniformBuffer, 
		StorageBuffer, 
		UniformBufferDynamic,
		StorageBufferDynamic, 
		InputAttachment
	};

	enum class BindingVisibility {
		Vertex = 0x00000001,
		Fragment = 0x00000010,
		Compute = 0x00000020,
		VertexFragment = Vertex | Fragment
	};

	struct PipelineLayoutDescriptor {

		struct LayoutBindingDesc {
			std::uint32_t binding = 0;
			std::uint32_t count = 1;
			bool is_bindless = false;
			BindingType type;
			BindingVisibility stage_flags;
			bool writable = false;
		};
		
		std::vector<LayoutBindingDesc> bindings;

		struct ConstantConfig {
			std::size_t size_in_bytes;
			std::uint8_t register_num = 0;
			StageVisibility visibility;

			ConstantConfig(std::size_t size_in_bytes_, std::uint8_t register_num_, StageVisibility visibility)
				: size_in_bytes(size_in_bytes_), register_num(register_num_), visibility(visibility) {}

			template <class T>
			ConstantConfig(T const& value, std::uint8_t register_num_, StageVisibility visibility) 
				: ConstantConfig(sizeof(T), (register_num_), visibility) {}

		};

		std::vector<ConstantConfig> constants;
	
	};

	export class IPipelineLayout
		: public plastic::utility::DisableCopy<IPipelineLayout> {
	
	};

}