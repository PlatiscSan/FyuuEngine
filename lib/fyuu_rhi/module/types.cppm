/* types.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <array>
#include <string>
#include <variant>
#include <optional>
#include <string_view>
#include <span>
#include <limits>
#if defined(__cpp_lib_inplace_vector)
#include <inplace_vector>
#endif // defined(__cpp_lib_inplace_vector)
#endif // !defined(__cpp_lib_modules)
#if !defined(__cpp_lib_inplace_vector)
#include <boost/container/static_vector.hpp>
#endif // !defined(__cpp_lib_inplace_vector)
#include "platform.hpp"
#define DECLARE_POLYMORPHIC_TYPE(NAME)	\
namespace vulkan {  \
	class Vulkan##NAME; \
}   \
\
namespace d3d12 {	\
	class D3D12##NAME;	\
}	\
\
namespace metal {	\
	class Metal##NAME;	\
}	\
\
namespace opengl {	\
	class OpenGL##NAME;	\
}	\
\
namespace webgpu {	\
	class WebGPU##NAME;	\
}

#if !defined(__APPLE__)
#define VULKAN_OPENGL_TYPES(NAME) , class vulkan::Vulkan##NAME, class opengl::OpenGL##NAME
#else
#define VULKAN_OPENGL_TYPES(NAME)
#endif

#if defined(_WIN32)
#define D3D12_TYPE(NAME) , class d3d12::D3D12##NAME
#else
#define D3D12_TYPE(NAME)
#endif

#if defined(__APPLE__)
#define METAL_TYPE(NAME) , class metal::Metal##NAME
#else
#define METAL_TYPE(NAME)
#endif

#define DECLARE_POLYMORPHIC_BASE(NAME)	\
	using Polymorphic##NAME##Base = plastic::utility::PolymorphicBase<\
		std::size_t	\
		VULKAN_OPENGL_TYPES(NAME)	\
		D3D12_TYPE(NAME)	\
		METAL_TYPE(NAME)	\
		, class webgpu::WebGPU##NAME	\
	>;	\
	using Unique##NAME = plastic::utility::UniqueBase<Polymorphic##NAME##Base>;

#define DECLARE_POLYMORPHIC(NAME) \
DECLARE_POLYMORPHIC_TYPE(NAME)	\
DECLARE_POLYMORPHIC_BASE(NAME)

export module fyuu_rhi:types;
#if defined(__cpp_lib_modules)
import std; 
#endif
import plastic.sealed_polymorphism;
import :enums;

export namespace fyuu_rhi {

#if defined(__cpp_lib_inplace_vector)
	template <class T, std::size_t N>
	using SmallVector = std::inplace_vector<T, N>;
#else
	template <class T, std::size_t N>
	using SmallVector = boost::container::static_vector<T, N>;
#endif // defined(__cpp_lib_inplace_vector)

	struct Version {
		std::uint8_t variant = 0;
		std::uint8_t major = 0;
		std::uint8_t minor = 0;
		std::uint8_t patch = 0;
	};

#if defined(__linux__)
	struct Wayland {
		wl_display* display;
		wl_surface* surface;
	};

	struct Xlib {
		Display* display;
		Window window;
	};
#endif // defined(__linux__)

	struct LogCallback {
		void(*Func)(LogSeverity, std::string_view, void*);
		void* user_data;
	};

	struct PlatformHandle {
#if defined(_WIN32)
		HWND impl;
#elif defined(__linux__)
		std::variant<std::monostate, Wayland, Xlib> impl;
#elif defined(__ANDROID__)
		ANativeWindow* impl;
#endif // defined(_WIN32)
	};

	struct InitOptions {

		/// @brief Name of app, required by Vulkan.
		std::string_view app_name;

		/// @brief Name of engine, required by Vulkan.
		std::string_view engine_name;

		/// @brief The version number of app, required by Vulkan.
		Version app_version;

		/// @brief The version number of engine, required by Vulkan.
		Version engine_version;

		/// @brief log callback
		LogCallback log_callback;

		/// @brief platform handle, required by OpenGL.
		PlatformHandle platform_handle;

		/// @brief Flag indicating whether a software fallback should be used if no compatible rendering device.
		bool software_fallback = false;

	};

	using BufferSize = std::size_t;

	struct TextureSize {
		std::size_t width;
		std::size_t height;
		std::size_t depth;

		TextureSize() = default;
		TextureSize(std::size_t width_, std::size_t height_, std::size_t depth_)
			: width(width_), height(height_), depth(depth_) {

		}
		
		TextureSize(TextureSize const&) noexcept = default;
		TextureSize(TextureSize&& other) noexcept
			: width(std::exchange(other.width, 0u)),
			height(std::exchange(other.height, 0u)),
			depth(std::exchange(other.depth, 0u)) {

		}

	};

	struct SwapChainOption {
		std::size_t back_buffer_size = 3u;
		ResourceFlags back_buffer_flags = ResourceFlags::RenderTarget | ResourceFlags::Texture2D | ResourceFlags::RGBA8_Unorm;
		bool enable_v_sync = true;
	};

	struct ShaderCompilationOption {
		std::string entry_point;
		HLSLOption hlsl_option;
		OptimizationLevel optimization;
	};

	struct SamplerAttachmentInfo {
		float mip_lod_bias = 0.0f;
		float min_lod = 0.0f;
		float max_lod = (std::numeric_limits<float>::max)();
	};

	struct BlendInfo {

		BlendFactor src_color_blend_factor = BlendFactor::One;
		BlendFactor dst_color_blend_factor = BlendFactor::Zero;
		BlendOp color_blend_op = BlendOp::Add;
	
		BlendFactor src_alpha_blend_factor = BlendFactor::One;
		BlendFactor dst_alpha_blend_factor = BlendFactor::Zero;
		BlendOp alpha_blend_op = BlendOp::Add;

		constexpr std::strong_ordering operator<=>(BlendInfo const& other) const noexcept = default;

	};

	struct RenderTargetAttachmentsInfo {
		std::variant<std::monostate, std::vector<BlendInfo>, LogicOp> blend_state;
		std::vector<std::uint8_t> color_write_masks;
	};

	struct DepthBias {
		float constant_factor = 0.0f, slope_factor = 0.0f, clamp = 0.0f;
	};

	struct StencilOpState {
		StencilOp failed;
		StencilOp depth_failed;
		StencilOp pass;
		CompareOp compare;
	};

	struct DepthStencilInfo {
		struct Depth {
			CompareOp compare;
			bool enable_write;
		};
		struct Stencil {
			std::uint8_t read_mask;
			std::uint8_t write_mask;
			StencilOpState front;
			StencilOpState back;
		};
		std::optional<Depth> depth;
		std::optional<Stencil> stencil;
		ResourceFlags format;
	};

	struct MultiSample {
		std::uint32_t count = 1u;
		std::uint32_t mask = 0xFFFFFFFF;
		bool enable_alpha_to_coverage = false;
	};

	struct SemanticInfo {
		std::string_view name;
		std::size_t index;
	};

	using VertexAttributeIdentifier = std::variant<std::size_t, SemanticInfo>;

	struct VertexBinding {
		std::size_t where;
		std::size_t stride;
		InputRate input_rate;
	};

	struct VertexAttribute {
		VertexAttributeIdentifier id;
		std::size_t where;
		std::size_t offset;
		ResourceFlags format;
	};

	struct VertexInputLayout {
		std::vector<VertexBinding> bindings;
		std::vector<VertexAttribute> attributes;
	};

	struct CmdSetFrame;
	struct CmdBeginRendering;
	struct CmdEndRendering;
	struct CmdExecute;
	struct CmdDispatchComputingTask;
	struct CmdSetViewports;
	struct CmdSetScissors;
	struct CmdBindVertexBuffers;
	struct CmdBindIndexBuffer;
	struct CmdDrawInstanced;
	struct CmdDrawIndexedInstanced;
	struct CmdBindBuffer;
	struct CmdPushData;
	struct CmdBindSampler;
	struct CmdBindTexture;
	struct CmdCopyTextureToBuffer;
	struct CmdCopyBufferToTexture;
	struct CmdCopyTextureToTexture;
	struct CmdExecuteIndirect;
	struct CmdExecuteIndirectIndexed;
	struct CmdDispatchIndirect;

	using PolymorphicCommandBase = plastic::utility::PolymorphicBase<
		std::uint64_t,
		struct CmdSetFrame,
		struct CmdBeginRendering,
		struct CmdEndRendering,
		struct CmdExecute,
		struct CmdDispatchComputingTask,
		struct CmdSetViewports,
		struct CmdSetScissors,
		struct CmdBindVertexBuffers,
		struct CmdBindIndexBuffer,
		struct CmdDrawInstanced,
		struct CmdDrawIndexedInstanced,
		struct CmdBindBuffer,
		struct CmdPushData,
		struct CmdBindSampler,
		struct CmdBindTexture,
		struct CmdCopyTextureToBuffer,
		struct CmdCopyBufferToTexture,
		struct CmdCopyTextureToTexture,
		struct CmdExecuteIndirect,
		struct CmdExecuteIndirectIndexed,
		struct CmdDispatchIndirect
	>;

#define IMPL_CMD_CTOR(COMMAND)	\
		COMMAND() : PolymorphicCommandBase(this) {}

	struct CmdSetFrame : PolymorphicCommandBase {
		std::optional<std::size_t> texture_id;
		std::optional<std::size_t> view_id;
		
		IMPL_CMD_CTOR(CmdSetFrame)
	};

	struct CmdBeginRendering : PolymorphicCommandBase {

		struct RenderTarget {
			std::optional<std::size_t> texture_id;
			std::optional<std::size_t> view_id;
			std::array<float, 4u> clear;
			LoadOp load;
			StoreOp store;
		};

		struct DepthStencil {
			std::optional<std::size_t> texture_id;
			std::optional<std::size_t> view_id;
			float depth_clear;
			std::uint8_t stencil_clear;
			LoadOp d_load;
			StoreOp d_store;
			LoadOp s_load;
			StoreOp s_store;
		};

		std::vector<RenderTarget> render_targets;
		std::optional<DepthStencil> depth_stencil;
		
		IMPL_CMD_CTOR(CmdBeginRendering)
	};

	struct CmdEndRendering : PolymorphicCommandBase {
		IMPL_CMD_CTOR(CmdEndRendering)
	};

	struct CmdExecute : PolymorphicCommandBase {
		std::optional<std::size_t> gpu_exec_id;
		IMPL_CMD_CTOR(CmdExecute)
	};

	struct CmdDispatchComputingTask : PolymorphicCommandBase {
		std::size_t threads_x;
		std::size_t threads_y;
		std::size_t threads_z;
		std::size_t threads_per_thread_group_x;
		std::size_t threads_per_thread_group_y;
		std::size_t threads_per_thread_group_z;
		IMPL_CMD_CTOR(CmdDispatchComputingTask)
	};

	struct CmdSetViewports : PolymorphicCommandBase {
		struct Viewport {
			float
				x = 0.0f,
				y = 0.0f,
				width = 1.0f,
				height = 1.0f,
				min_depth = 0.0f,
				max_depth = 1.0f;
		};
		std::vector<Viewport> viewports;
		IMPL_CMD_CTOR(CmdSetViewports)
	};

	struct CmdSetScissors : PolymorphicCommandBase {
		struct Scissor {
			std::ptrdiff_t x;
			std::ptrdiff_t y;
			std::size_t width;
			std::size_t height;
		};
		std::vector<Scissor> scissors;
		IMPL_CMD_CTOR(CmdSetScissors)
	};

	struct CmdBindVertexBuffers : PolymorphicCommandBase {
		struct BindingDescription {
			std::optional<std::size_t> id;
			std::size_t where;
			std::size_t offset;
			std::size_t stride;
		};
		std::vector<BindingDescription> bindings;
		IMPL_CMD_CTOR(CmdBindVertexBuffers)
	};
	
	struct CmdBindIndexBuffer : PolymorphicCommandBase {
		std::optional<std::size_t> id;
		IMPL_CMD_CTOR(CmdBindIndexBuffer)
	};

	struct CmdDrawInstanced : PolymorphicCommandBase {
		std::size_t vertices;
		std::size_t instances;
		std::size_t start_vertex;
		std::size_t first_instance;
		IMPL_CMD_CTOR(CmdDrawInstanced)
	};

	struct CmdDrawIndexedInstanced : PolymorphicCommandBase {
		std::size_t indices;
		std::size_t instances;
		std::size_t first_index;
		std::size_t start_vertex;
		std::size_t first_instance;
		IMPL_CMD_CTOR(CmdDrawIndexedInstanced)
	};

	struct CmdBindBuffer : PolymorphicCommandBase {
		std::optional<std::size_t> id;
		std::optional<std::size_t> texel_view_id;
		std::size_t offset;
		std::size_t where;
		std::size_t ns;
		ShaderStage stages;
		IMPL_CMD_CTOR(CmdBindBuffer)
	};

	struct CmdPushData : PolymorphicCommandBase {
		std::size_t where;
		std::size_t ns;
		std::size_t offset;
		std::vector<std::byte> bytes;
		ShaderStage stages;
		IMPL_CMD_CTOR(CmdPushData)
	};

	struct CmdBindSampler : PolymorphicCommandBase {
		std::optional<std::size_t> id;
		std::size_t where;
		std::size_t ns;
		ShaderStage stages;
		IMPL_CMD_CTOR(CmdBindSampler)
	};

	struct CmdBindTexture : PolymorphicCommandBase {
		std::optional<std::size_t> texture_id;
		std::optional<std::size_t> view_id;
		std::size_t where;
		std::size_t ns;
		ShaderStage stages;
		IMPL_CMD_CTOR(CmdBindTexture)
	};

	struct CmdCopyFamily : PolymorphicCommandBase {
		struct Subresource {
			std::size_t mip_level;
			std::size_t array_layer;
			std::size_t layer_count;
		};

		struct Offset3D {
			std::ptrdiff_t x;
			std::ptrdiff_t y;
			std::ptrdiff_t z;
		};

		struct Extent3D {
			std::size_t width;
			std::size_t height;
			std::size_t depth;
		};

		std::optional<std::size_t> src_id;
		std::optional<std::size_t> dest_id;

		template <std::derived_from<CmdCopyFamily> Derived>
		CmdCopyFamily(Derived* derived) : PolymorphicCommandBase(derived) {

		}

	};

#define IMPL_COPY_CMD_CTOR(COMMAND)	\
		COMMAND() : CmdCopyFamily(this) {}	

	struct CmdCopyTextureToBuffer : CmdCopyFamily {
		Subresource src_subresource;
		Offset3D src_offset;
		std::size_t dest_offset;
		Extent3D extent;

		IMPL_COPY_CMD_CTOR(CmdCopyTextureToBuffer)
	};
	
	struct CmdCopyBufferToTexture : CmdCopyFamily {
		Subresource dest_subresource;
		Offset3D dest_offset;
		std::size_t src_offset;
		Extent3D extent;
	
		IMPL_COPY_CMD_CTOR(CmdCopyBufferToTexture)
	};
	
	struct CmdCopyBufferToBuffer : CmdCopyFamily {
		std::size_t src_offset;
		std::size_t dest_offset;
		std::size_t size;
	
		IMPL_COPY_CMD_CTOR(CmdCopyBufferToBuffer)
	};
	
	struct CmdCopyTextureToTexture : CmdCopyFamily {
		Subresource src_subresource;
		Subresource dest_subresource;
		Offset3D src_offset;
		Offset3D dest_offset;
		Extent3D extent;
	
		IMPL_COPY_CMD_CTOR(CmdCopyTextureToTexture)
	};

	struct CmdExecuteIndirect : PolymorphicCommandBase {
		std::optional<std::size_t> id;
		std::size_t offset_into_buffer;
		std::size_t draws;
		IMPL_CMD_CTOR(CmdExecuteIndirect)
	};

	struct CmdExecuteIndirectIndexed : PolymorphicCommandBase {
		std::optional<std::size_t> id;
		std::size_t offset_into_buffer;
		std::size_t draws;
		IMPL_CMD_CTOR(CmdExecuteIndirectIndexed)
	};

	struct CmdDispatchIndirect : PolymorphicCommandBase {
		std::optional<std::size_t> id;
		std::size_t offset_into_buffer;
		std::size_t block_size_x;
		std::size_t block_size_y;
		std::size_t block_size_z;
		IMPL_CMD_CTOR(CmdDispatchIndirect)
	};

	using UniqueCommand = plastic::utility::UniqueBase<PolymorphicCommandBase>;

	DECLARE_POLYMORPHIC(PhysicalDevice)
	DECLARE_POLYMORPHIC(LogicalDevice)
	DECLARE_POLYMORPHIC(Future)
	DECLARE_POLYMORPHIC(Promise)
	DECLARE_POLYMORPHIC(Resource)
	DECLARE_POLYMORPHIC(View)
	DECLARE_POLYMORPHIC(Shader)
	DECLARE_POLYMORPHIC(Sampler)
	DECLARE_POLYMORPHIC(ShaderDataSegment)
	DECLARE_POLYMORPHIC(GPUExecutable)
	DECLARE_POLYMORPHIC(CommandBuffer)
	DECLARE_POLYMORPHIC(CommandQueue)
	DECLARE_POLYMORPHIC(Surface)
	DECLARE_POLYMORPHIC(SwapChain)

} // namespace fyuu_rhi
