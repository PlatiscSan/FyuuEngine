export module spirv:interface;
import std;

namespace fyuu_engine::spirv {

	export using SPIRVCode = std::vector<std::uint32_t>;
	export using SPIRVCodeView = std::span<std::uint32_t>;

	export using SPIRVBinary = std::vector<std::byte>;
	export using SPIRVBinaryView = std::span<std::byte>;

	export enum class ShaderStage : std::uint8_t {

		/*	Traditional pipeline */

		Vertex,
		TessellationControl,
		TessellationEvaluation,
		Geometry,
		Fragment,

		Compute,

		/* Ray tracing */

		RayGeneration,
		AnyHit,
		ClosestHit,
		Miss,
		Intersection,
		Callable,

		/*	Mesh shading */

		Task,
		Mesh,

		Kernel, // OpenCL kernel

		Count
	};

	export struct EntryPoint {
		std::string name;
		ShaderStage stage;
	};

	export enum class ResourceType : std::uint8_t {
		UniformBuffer,
		StorageBuffer,
		PushConstant,
		SampledImage,
		StorageImage,
		Sampler,
		SeparateImage,
		SeparateSampler,
		AccelerationStructure,
		InputAttachment,
		SubpassInput,
		Count
	};

	export struct ResourceDescription {
		std::uint32_t set;
		std::uint32_t binding;
		std::uint32_t count;
		std::uint32_t size;
		std::string name;
		ShaderStage stage;	
		ResourceType type;
	};

	export struct TypeDescription {
		std::string name;
		std::uint32_t size;
		std::uint32_t alignment;
		std::uint32_t array_size;
		bool is_array;
	};

	export struct IOVariable {
		std::string name;
		std::uint32_t location;
		std::uint32_t component_count;
		std::string type;
		ShaderStage stage;
	};

	export struct DescriptorSetLayout {
		std::uint32_t set;
		std::vector<ResourceDescription> resources;
		std::uint32_t push_constant_size;
	};

	export struct ReflectionInfo {
		std::string entry_point;

		std::vector<DescriptorSetLayout> descriptor_sets;
		std::vector<IOVariable> input_variables;
		std::vector<IOVariable> output_variables;
		std::vector<ResourceDescription> resources;
		std::unordered_map<std::string, TypeDescription> types;

		struct {
			uint32_t local_size_x, local_size_y, local_size_z;
		} specialization;

		ShaderStage stage;

	};

	export struct CompileOptions {

		std::string entry_point = "main";

		std::vector<std::string> defines;
		std::unordered_map<std::string, std::string> include_directories;

		struct {
			bool vulkan_glsl = false;
			bool opengl_glsl = false;
			bool hlsl = false;
			bool msl = false;  // Metal Shading Language
			std::uint32_t version = 450;
		} target;

		ShaderStage target_stage;
		bool optimize = false;
		bool generate_debug_info = false;

	};

	export struct CrossCompileOptions {

		std::string entry_point;
		std::uint32_t version = 450;

		enum class TargetLanguage : std::uint8_t {
			GLSL,
			HLSL,
			MSL,
			CPP
		} target_language = TargetLanguage::GLSL;


		bool version_es = false;
		bool bindless = false;

	};

	export class IReflection {
	public:
		virtual ~IReflection() = default;

		virtual std::vector<EntryPoint> QueryEntryPoints() const = 0;

		virtual std::vector<ResourceDescription> QueryResources(EntryPoint const& entry) = 0;
		virtual std::vector<DescriptorSetLayout> QueryDescriptorSetLayouts(EntryPoint const& entry) = 0;

		virtual std::vector<IOVariable> QueryInputVariables(EntryPoint const& entry) = 0;
		virtual std::vector<IOVariable> QueryOutputVariables(EntryPoint const& entry) = 0;

	};

	export class ICompiler {
	public:
		virtual ~ICompiler() = default;

		virtual SPIRVCode CompileFromSource(
			std::string_view source_code,
			const CompileOptions& options
		) = 0;

		virtual SPIRVCode CompileFromFile(
			std::string_view file_path,
			const CompileOptions& options
		) = 0;

		virtual std::string CrossCompile(
			SPIRVCodeView spirv,
			const CrossCompileOptions& options
		) = 0;

		virtual SPIRVCode Optimize(
			SPIRVCodeView spirv,
			bool strip_debug_info = false
		) = 0;

		virtual bool Validate(SPIRVCodeView spirv) = 0;
		virtual std::string GetValidationMessages(SPIRVCodeView spirv) = 0;

		virtual std::string Disassemble(SPIRVCodeView spirv) = 0;
		virtual SPIRVCode Assemble(std::string_view assembly) = 0;
	};

}