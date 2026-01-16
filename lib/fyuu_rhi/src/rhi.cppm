module;

export module fyuu_rhi;
import std;

export import :device;
export import :pipeline;

#if defined(_WIN32)
export import :d3d12_physical_device;
export import :d3d12_logical_device;
export import :d3d12_command_object;
export import :d3d12_swap_chain;
export import :d3d12_synchronization;
export import :d3d12_shader_library;
export import :d3d12_memory;
export import :d3d12_resource;
export import :d3d12_view;
#endif // defined(_WIN32)

export import :vulkan_physical_device;
export import :vulkan_logical_device;
export import :vulkan_command_object;
export import :vulkan_surface;
export import :vulkan_swap_chain;
export import :vulkan_shader_library;
export import :vulkan_memory;
export import :vulkan_resource;
export import :vulkan_view;

namespace fyuu_rhi {

	export enum class API : std::uint8_t {
		Unknown,				// this flag does nothing
		PlatformDefault,		// automatically pick an API
		Vulkan,					// Vulkan
		DirectX12,				// Microsoft DirectX 12
		Metal,					// Apple Metal
		OpenGL,					// OpenGL
	};

	namespace details {
		
		template <API api> struct APITraits {
			struct PhysicalDevice {};
			struct LogicalDevice {};
			struct CommandQueue {};
			struct Surface {};
			struct SwapChain {};
			struct ShaderLibrary {};
			struct VideoMemory {};
			struct Resource {};

		};

		template<> struct APITraits<API::Vulkan> {
			using PhysicalDevice = fyuu_rhi::vulkan::VulkanPhysicalDevice;
			using LogicalDevice = fyuu_rhi::vulkan::VulkanLogicalDevice;
			using CommandQueue = fyuu_rhi::vulkan::VulkanCommandQueue;
			using Surface = fyuu_rhi::vulkan::VulkanSurface;
			using SwapChain = fyuu_rhi::vulkan::VulkanSwapChain;
			using ShaderLibrary = fyuu_rhi::vulkan::VulkanShaderLibrary;
			using VideoMemory = fyuu_rhi::vulkan::VulkanVideoMemory;
			using Resource = fyuu_rhi::vulkan::VulkanResource;
		};


#if defined(_WIN32)
		template<> struct APITraits<API::DirectX12> {
			using PhysicalDevice = fyuu_rhi::d3d12::D3D12PhysicalDevice;
			using LogicalDevice = fyuu_rhi::d3d12::D3D12LogicalDevice;
			using CommandQueue = fyuu_rhi::d3d12::D3D12CommandQueue;
			using Surface = fyuu_rhi::d3d12::D3D12Surface;
			using SwapChain = fyuu_rhi::d3d12::D3D12SwapChain;
			using ShaderLibrary = fyuu_rhi::d3d12::D3D12ShaderLibrary;
			using VideoMemory = fyuu_rhi::d3d12::D3D12VideoMemory;
			using Resource = fyuu_rhi::d3d12::D3D12Resource;
		};
#endif

		template <class Variant, class T>
		struct RHITypeIndexImpl;

		template <class T, class... Types>
		struct RHITypeIndexImpl<std::variant<Types...>, T> {
			static constexpr std::size_t match_count = (std::is_same_v<T, Types> +...);

			// Intercept all errors at compile time. 
			// When the compiler executes the following code, it has already determined that the match number is valid!
			// Target type not found → Compilation error
			static_assert(match_count != 0,
				"❌ Target type not found in std::variant type list!");

			// Target type appears repeatedly → Compilation error (Required in RHI layer, variant type must be unique)
			static_assert(match_count == 1,
				"❌ Target type appears multiple times in std::variant!");

			template <std::size_t... Idx>
			static consteval std::size_t Index(std::index_sequence<Idx...>) noexcept {
				// Construct an array at compile time, assigning Idx to matches and 0 to non-matches
				// (0 has no effect because the assertion guarantees a unique match)
				constexpr std::size_t indices[] = { std::is_same_v<T, Types> ? Idx : 0 ... };
				// Compile-time summation: The uniquely matching Idx is retained, and the rest are 0. 
				// The summation result is the target index.
				return (indices[Idx] + ...);
			}

			static constexpr std::size_t Value = Index(std::make_index_sequence<sizeof...(Types)>{});
		};

		/*template <class Variant, class T>
		struct RHITypeIndexImpl;

		template <class T, class... Types>
		struct RHITypeIndexImpl<std::variant<Types...>, T> {
			template <std::size_t... Is>
			static constexpr std::size_t Index(std::index_sequence<Is...>) {
				constexpr bool matches[] = { std::is_same_v<T, Types>... };
				for (std::size_t i = 0; i < sizeof...(Types); ++i) {
					if (matches[i]) {
						return i;
					}
				}
				return static_cast<std::size_t>(-1);
			}

			static constexpr std::size_t Value = Index(
				std::make_index_sequence<sizeof...(Types)>{}
			);
		};*/

	}

#if defined(_WIN32)
#define DECLARE_RHI_TYPE(TYPE) export using TYPE = std::variant<std::monostate, details::APITraits<API::DirectX12>::TYPE, details::APITraits<API::Vulkan>::TYPE>
#elif defined(__linux__) || defined(__ANDROID__)
#define DECLARE_RHI_TYPE(TYPE) export using TYPE = std::variant<std::monostate, details::APITraits<API::Vulkan>::TYPE>
#endif // defined(_WIN32)

	DECLARE_RHI_TYPE(PhysicalDevice);
	DECLARE_RHI_TYPE(LogicalDevice);
	DECLARE_RHI_TYPE(CommandQueue);
	DECLARE_RHI_TYPE(Surface);
	DECLARE_RHI_TYPE(SwapChain);
	DECLARE_RHI_TYPE(ShaderLibrary);
	DECLARE_RHI_TYPE(VideoMemory);
	DECLARE_RHI_TYPE(Resource);

#undef DECLARE_RHI_TYPE

	export template <class Variant, class T> inline constexpr std::size_t RHITypeIndex = details::RHITypeIndexImpl<Variant, T>::Value;

	export std::span<API const> QueryAvailableAPI() noexcept;


}