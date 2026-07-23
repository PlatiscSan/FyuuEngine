/* the d3d12 pattern
module;
#include <version>
#if !defined(__cpp_lib_modules)

#endif // !defined(__cpp_lib_modules)
#if defined(_WIN32)
#include <dxgi1_3.h>
#include <d3d12.h>
#include <wrl.h>
#endif // defined(_WIN32)
export module fyuu_rhi:;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
namespace fyuu_rhi::d3d12 {

}
#endif // defined(_WIN32)

*/

module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstddef>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include <variant>
#include <string_view>
#include <span>
#include <cstdint>
#endif // !defined(__cpp_lib_modules)
#if defined(_WIN32)
#include <dxgi1_3.h>
#include <D3D12MemAlloc.h>
#include <dxcapi.h>
#include <wrl.h>
#endif // defined(_WIN32)
export module fyuu_rhi:d3d12_traits;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :core_types;
import :resource_types;
import :d3d12_utility;
import :d3d12_descriptor_allocator;
import :d3d12_device_removal_tracker;
import :sampler_types;
import :scheduler_types;
import :pipeline_types;
import :native_pipeline_binding;

namespace fyuu_rhi::d3d12 {

	using namespace fyuu_rhi::pipeline;
	using namespace fyuu_rhi::execution;

	export struct Backend {

		using Instance = Microsoft::WRL::ComPtr<IDXGIFactory2>;
		using PhysicalDevice = Microsoft::WRL::ComPtr<IDXGIAdapter1>;

		struct LogicalDevice {
			Microsoft::WRL::ComPtr<IDXGIAdapter1> phys_dev;
			Microsoft::WRL::ComPtr<ID3D12Device> impl;
			DeviceRemovalTracker rm_tracker;
			Microsoft::WRL::ComPtr<D3D12MA::Allocator> mem_alloc;
			DescriptorAllocator univ_alloc;
			DescriptorAllocator rtv_alloc;
			DescriptorAllocator dsv_alloc;
			DescriptorAllocator sampler_alloc;
			Microsoft::WRL::ComPtr<ID3D12CommandSignature> multidraw;
			Microsoft::WRL::ComPtr<ID3D12CommandSignature> multidraw_indexed;
			Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatch_indirect;
		};

		struct Scheduler {
			struct Implementation {
				Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
				Microsoft::WRL::ComPtr<ID3D12Fence> fence;
				std::atomic_uint64_t next_fence_value;
			};
			std::shared_ptr<Implementation> impl;
		};

		struct Resource {
			struct Implementation {
				Microsoft::WRL::ComPtr<D3D12MA::Allocation> alloc;
				D3D12_RESOURCE_STATES last_state;
				D3D12_RESOURCE_STATES curr_state;
			};

			std::shared_ptr<Implementation> impl;

		};

		using View = std::shared_ptr<ManagedDescriptorHandle>;

		using Sampler = std::shared_ptr<ManagedDescriptorHandle>;

		struct Pipeline {
			Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
			Microsoft::WRL::ComPtr<ID3D12PipelineState> state;
			D3D_PRIMITIVE_TOPOLOGY primitive_topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
			std::vector<PipelineBindingMetadata> bindings;
		};

		using PipelineResourceGroup = NativePipelineResourceGroup<Backend>;

		static Microsoft::WRL::ComPtr<IDXGIFactory2> CreateInstance(std::string_view app_name, Version const& app_ver, std::string_view engine_name, Version const& engine_ver);

		static std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>> EnumeratePhysicalDevices(Microsoft::WRL::ComPtr<IDXGIFactory2> const& factory);

		static PhysicalDeviceInfo GetPhysicalDeviceInfo(Microsoft::WRL::ComPtr<IDXGIAdapter1> const& adapter);

		static LogicalDevice CreateLogicalDevice(Microsoft::WRL::ComPtr<IDXGIAdapter1> const& adapter);

		static Scheduler CreateScheduler(LogicalDevice const& ld, SchedulerDescriptor const& descriptor);

		static Resource CreateBuffer(LogicalDevice const& ld, std::size_t size_in_bytes, ResourceFlags const& flags);

		static Resource CreateTexture(LogicalDevice const& ld, std::size_t width, std::size_t height, std::size_t depth_arr_layers, std::size_t mip_lvl_cnt, ResourceFlags const& flags);

		static std::shared_ptr<ManagedDescriptorHandle> CreateTextureView(LogicalDevice& ld, Resource const& res, std::size_t base_mip_lvl, std::size_t mip_lvl_cnt, std::size_t base_arr_layer, std::size_t arr_layer_cnt, ResourceFlags const& flags);

		static std::shared_ptr<ManagedDescriptorHandle> CreateBufferView(LogicalDevice& ld, Resource const& res, std::size_t offset, std::size_t range, ResourceFlags const& flags);

		static std::shared_ptr<ManagedDescriptorHandle> CreateSampler(LogicalDevice& ld, SamplerDescriptor const& descriptor);

		static Pipeline CreateGraphicsPipeline(LogicalDevice const& ld, GraphicsPipelineDescriptor const& desc);

		static PipelineResourceGroup CreatePipelineResourceGroup(
			LogicalDevice const& ld,
			Pipeline const& pipeline,
			std::uint32_t space,
			std::span<NativePipelineResourceBinding<Backend> const> bindings
		);

	};

}
#endif // defined(_WIN32)
