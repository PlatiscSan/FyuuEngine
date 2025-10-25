export module rendering:shader;

export import coroutine;
export import scheduler;
import std;
import file;

namespace fyuu_engine::core {
	
	export enum class ShaderStage : std::uint8_t {

		/*	Traditional pipeline */

		Vertex,
		TessellationControl,
		TessellationEvaluation,
		Geometry,
		Fragment,
		Pixel = Fragment,

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

		Unknown,

		Count
	};

	export enum class ShaderLanguage : std::uint8_t {
		Unknown,
		GLSL,
		HLSL,
		Count,
	};

	export struct CompilationOption {
		std::vector<std::string> definitions;
		std::unordered_map<std::string, std::string> include_directories;
		std::string entry_point = "main";
		ShaderStage target = ShaderStage::Unknown;
		ShaderLanguage language = ShaderLanguage::Unknown;
		bool optimize = false;
		bool debug_info = false;
	};

	export template <class Func>
		concept ShaderCompiledCallback = requires(Func f, std::vector<std::byte> result) {
			{ f(result) } -> std::same_as<void>;
	};

	export template <class DerivedCompiler> class BaseShaderCompiler {
	public:
		std::vector<std::byte> CompileFromSource(std::span<std::byte> source_code, CompilationOption const& option) {
			return static_cast<DerivedCompiler*>(this)->CompileImpl(source_code, option);
		}

		concurrency::AsyncTask<std::vector<std::byte>> AsyncCompileFromSource(std::span<std::byte> source_code, CompilationOption const& option, concurrency::Scheduler* scheduler) {
			
			std::future<std::vector<std::byte>> result = co_await scheduler->WaitForTasks(
				[this, source_code, option]() {
					return CompileFromSource(source_code, option);
				}
			);

			co_return result.get();

		}

		template <ShaderCompiledCallback Func>
		concurrency::AsyncTask<void> AsyncCompileFromSource(std::span<std::byte> source_code, CompilationOption const& option, concurrency::Scheduler* scheduler, Func&& func) {
			
			std::future<std::vector<std::byte>> result = co_await scheduler->WaitForTasks(
				[this, source_code, option]() {
					return CompileFromSource(source_code, option);
				}
			);

			func(result.get());

		}

		std::vector<std::byte> CompileFromFile(std::filesystem::path const& src_path, CompilationOption const& option) {
			return CompileFromSource(util::ReadFile(src_path), option);
		}

		concurrency::AsyncTask<std::vector<std::byte>> AsyncCompileFromSource(std::filesystem::path const& src_path, CompilationOption const& option, concurrency::Scheduler* scheduler) {
			return AsyncCompileFromSource(util::ReadFile(src_path), option, scheduler);
		}

	};

	export template <class DerivedReflection> class BaseShaderReflection {
	public:
		void ReflectFromBinary(std::span<std::byte> binary_data) {
			static_cast<DerivedReflection*>(this)->ReflectImpl(binary_data);
		}

	};

}