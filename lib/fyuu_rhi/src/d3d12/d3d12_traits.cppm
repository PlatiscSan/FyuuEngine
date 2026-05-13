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
#include <vector>
#include <variant>
#endif // !defined(__cpp_lib_modules)
#if defined(_WIN32)
#include <dxgi1_3.h>
#include <D3D12MemAlloc.h>
#include <wrl.h>
#endif // defined(_WIN32)
#define BOOST_DISABLE_ASSERTS
#include <boost/locale.hpp>
export module fyuu_rhi:d3d12_traits;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :core_types;
import :command_types;
import :d3d12_utility;
import :d3d12_descriptor_allocator;
import :d3d12_device_removal_tracker;
import :resource_types;

namespace fyuu_rhi::d3d12 {

	export struct Backend {

		using Instance = Microsoft::WRL::ComPtr<IDXGIFactory2>;
		using PhysicalDevice = Microsoft::WRL::ComPtr<IDXGIAdapter1>;
		using Surface = HWND;

		struct LogicalDevice {
			Microsoft::WRL::ComPtr<ID3D12Device> impl;
			std::shared_ptr<DeviceRemovalTracker> rm_tracker;
			Microsoft::WRL::ComPtr<D3D12MA::Allocator> mem_alloc;
			D3D12DescriptorAllocator univ_alloc;
			D3D12DescriptorAllocator rtv_alloc;
			D3D12DescriptorAllocator dsv_alloc;
			D3D12DescriptorAllocator sampler_alloc;
			Microsoft::WRL::ComPtr<ID3D12CommandSignature> multidraw;
			Microsoft::WRL::ComPtr<ID3D12CommandSignature> multidraw_indexed;
			Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatch_indirect;
		};

		struct Future {
			struct FenceSynchronization {
				std::uint64_t fence_val;
				UniqueEvent fence_event;
			};
			Microsoft::WRL::ComPtr<ID3D12Fence> fence;
			std::shared_ptr<FenceSynchronization> sync;
		};

		struct Promise {
			Microsoft::WRL::ComPtr<ID3D12Fence> fence;
			TimelineCounter counter;
			Future last_fut;
		};

		using Resource = Microsoft::WRL::ComPtr<D3D12MA::Allocation>;

		using View = std::shared_ptr<D3D12DescriptorHandle>;

		static Microsoft::WRL::ComPtr<IDXGIFactory2> CreateInstance();

		static std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>> EnumeratePhysicalDevices(Microsoft::WRL::ComPtr<IDXGIFactory2> const& factory);

		static PhysicalDeviceInfo GetPhysicalDeviceInfo(Microsoft::WRL::ComPtr<IDXGIAdapter1> const& adapter);

		static HWND CreateSurface(Microsoft::WRL::ComPtr<IDXGIFactory2> const& factory, HWND window_handle) noexcept;

		static LogicalDevice CreateLogicalDevice(Microsoft::WRL::ComPtr<IDXGIAdapter1> const& adapter);

		static Promise CreatePromise(LogicalDevice const& ld);

		static Future GetFuture(Promise& promise) noexcept;

		static Microsoft::WRL::ComPtr<D3D12MA::Allocation> CreateBuffer(LogicalDevice const& ld, std::size_t size_in_bytes, ResourceFlags const& flags);

		static Microsoft::WRL::ComPtr<D3D12MA::Allocation> CreateTexture(LogicalDevice const& ld, std::size_t width, std::size_t height, std::size_t depth_arr_layers, std::size_t mip_lvl_cnt, ResourceFlags const& flags);

		static std::shared_ptr<D3D12DescriptorHandle> CreateTextureView(LogicalDevice const& ld, Microsoft::WRL::ComPtr<D3D12MA::Allocation> const& alloc, std::size_t base_mip_lvl, std::size_t mip_lvl_cnt, std::size_t base_arr_layer, std::size_t arr_layer_cnt, ResourceFlags const& flags);

	};

}
#endif // defined(_WIN32)