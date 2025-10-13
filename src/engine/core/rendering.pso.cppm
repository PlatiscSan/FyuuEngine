export module rendering:pso;
import std;
import defer;
export import disable_copy;
export import scheduler;

namespace fyuu_engine::core {

	export enum class PrimitiveTopology : std::uint8_t {
		Undefined,
		PointList,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip,
		Count
	};

	export enum class FillMode : std::uint8_t {
		Undefined,
		Wireframe,
		Solid,
		Count
	};

	export enum class CullMode : std::uint8_t {
		Undefined,
		None,
		Front,
		Back,
		Count
	};

	export enum class BlendOp : std::uint8_t {
		Undefined,
		Add,
		Subtract,
		ReverseSubtract,
		Min,
		Max,
		Count
	};

	export enum class BlendFactor : std::uint8_t {
		Undefined,
		Zero,
		One,
		SrcColor,
		InvSrcColor,
		SrcAlpha,
		InvSrcAlpha,
		DestAlpha,
		InvDestAlpha,
		DestColor,
		InvDestColor,
		SrcAlphaSat,
		BlendFactor,
		InvBlendFactor,
		Src1Color,
		InvSrc1Color,
		Src1Alpha,
		InvSrc1Alpha,
		Count
	};

	export enum class ComparisonFunc : std::uint8_t {
		Undefined,
		Never,
		Less,
		Equal,
		LessEqual,
		Greater,
		NotEqual,
		GreaterEqual,
		Always,
		Count
	};

	export enum class StencilOp : std::uint8_t {
		Undefined,
		Keep,
		Zero,
		Replace,
		IncrementSaturate,
		DecrementSaturate,
		Invert,
		Increment,
		Decrement,
		Count
	};

	export enum class ShaderStage : std::uint8_t {
		Unknown,
		Vertex,
		Fragment,
		Pixel = Fragment,
		Geometry,
		Mesh,
		All,
		Count
	};

	/// @brief 
	/// @tparam DerivedPipelineStateObjectBuilder 
	/// @tparam PSO like Microsoft::WRL::ComPtr<ID3D12PipelineState> and vk::UniquePipeline or whatever else
	export template <class DerivedPipelineStateObjectBuilder, class PSO> class BasePipelineStateObjectBuilder
		: public util::DisableCopy<BasePipelineStateObjectBuilder<DerivedPipelineStateObjectBuilder, PSO>> {
	private:
		static inline std::once_flag s_scheduler_init_flag;

	protected:
		static inline concurrency::Scheduler* s_scheduler;

	public:
		struct NoSchedulerFlag {};

		BasePipelineStateObjectBuilder() {
			std::call_once(
				s_scheduler_init_flag,
				[]() {
					static concurrency::Scheduler scheduler;

					/*
					*	 default to 4 workers
					*/

					static std::array workers = {
						concurrency::Worker(),
						concurrency::Worker(),
						concurrency::Worker(),
						concurrency::Worker()
					};

					s_scheduler = &scheduler;

					for (auto& worker : workers) {
						scheduler.Hire(util::MakeReferred(&worker));
					}
				}
			);
		}

		BasePipelineStateObjectBuilder(NoSchedulerFlag) {
			std::call_once(
				s_scheduler_init_flag,
				[]() {
				
				}
			);
		}

		/// @brief build the pso asynchronously if API supports
		/// @return 
		concurrency::AsyncTask<PSO> Build() const {
			return static_cast<DerivedPipelineStateObjectBuilder const*>(this)->BuildImpl();
		}

		DerivedPipelineStateObjectBuilder& LoadVertexShader(std::filesystem::path const& path) {
			return static_cast<DerivedPipelineStateObjectBuilder*>(this)->LoadVertexShaderImpl(path);
		}

		DerivedPipelineStateObjectBuilder& LoadFragmentShader(std::filesystem::path const& path) {
			return static_cast<DerivedPipelineStateObjectBuilder*>(this)->LoadFragmentShaderImpl(path);
		}

		DerivedPipelineStateObjectBuilder& LoadPixelShader(std::filesystem::path const& path) {
			return static_cast<DerivedPipelineStateObjectBuilder*>(this)->LoadPixelShaderImpl(path);
		}

		DerivedPipelineStateObjectBuilder& CompileVertexShader(std::filesystem::path const& path, std::string_view entry) {
			return static_cast<DerivedPipelineStateObjectBuilder*>(this)->CompileVertexShaderImpl(path, entry);
		}

		DerivedPipelineStateObjectBuilder& CompileFragmentShader(std::filesystem::path const& path, std::string_view entry) {
			return static_cast<DerivedPipelineStateObjectBuilder*>(this)->CompileFragmentShaderImpl(path, entry);
		}

		DerivedPipelineStateObjectBuilder& CompilePixelShader(std::filesystem::path const& path, std::string_view entry) {
			return static_cast<DerivedPipelineStateObjectBuilder*>(this)->CompilePixelShaderImpl(path, entry);
		}
	};

}