module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <array>
#endif // !defined(__cpp_lib_modules)
#if defined(_WIN32)
#include <dxgi1_3.h>
#include <D3D12MemAlloc.h>
#include <wrl.h>
#include <comdef.h>
#endif // defined(_WIN32)
#define BOOST_DISABLE_ASSERTS
#include <boost/locale.hpp>
module fyuu_rhi:d3d12_physical_device;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :d3d12_traits;
import :d3d12_utility;
import :d3d12_cache;

namespace fyuu_rhi::d3d12 {

	PhysicalDeviceInfo Backend::GetPhysicalDeviceInfo(Microsoft::WRL::ComPtr<IDXGIAdapter1> const& adapter) {

		using Type = PhysicalDeviceInfo::Type;
		auto GetType = [](DXGI_ADAPTER_DESC1& desc) {
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
				return Type::CPU;
			}
			else if (desc.DedicatedVideoMemory > 0) {
				return Type::DiscreteGPU;
			}
			else {
				return Type::IntegratedGPU;
			}
			};

		DXGI_ADAPTER_DESC1 desc;
		HRESULT result = adapter->GetDesc1(&desc);
		ThrowIfFailed(result);

		PhysicalDeviceInfo info = {
			.name = boost::locale::conv::utf_to_utf<char>(desc.Description),
			.vendor_id = desc.VendorId,
			.device_id = desc.DeviceId,
			.dedicated_memory = desc.DedicatedVideoMemory,
			.type = GetType(desc)
		};

		return info;
	}

	Backend::LogicalDevice Backend::CreateLogicalDevice(Microsoft::WRL::ComPtr<IDXGIAdapter1> const& adapter) {

		Microsoft::WRL::ComPtr<ID3D12Device> dev = [&adapter]() {
			Microsoft::WRL::ComPtr<ID3D12Device> dev;
			HRESULT result = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&dev));
			ThrowIfFailed(result);
			return dev;
			}();

		std::shared_ptr<DeviceRemovalTracker> rm_tracker = std::make_shared<DeviceRemovalTracker>(dev);

		Microsoft::WRL::ComPtr<D3D12MA::Allocator> mem_alloc = [&adapter, &dev]() {
			
			Microsoft::WRL::ComPtr<D3D12MA::Allocator> mem_alloc;

			D3D12MA::ALLOCATOR_DESC allocator_desc = {};
			allocator_desc.pDevice = dev.Get();
			allocator_desc.pAdapter = adapter.Get();

			HRESULT result = D3D12MA::CreateAllocator(&allocator_desc, &mem_alloc);
			ThrowIfFailed(result);

			return mem_alloc;

			}();

		D3D12DescriptorAllocator univ_alloc = CreateUniversalViewAllocator(dev.Get());
		D3D12DescriptorAllocator rtv_alloc = CreateRTVAllocator(dev.Get());
		D3D12DescriptorAllocator dsv_alloc = CreateDSVAllocator(dev.Get());
		D3D12DescriptorAllocator sampler_alloc = CreateSamplerAllocator(dev.Get());

		Microsoft::WRL::ComPtr<ID3D12CommandSignature> multidraw = [&dev]() {

			Microsoft::WRL::ComPtr<ID3D12CommandSignature> cmd_sgn;

			std::array argument_descs = {
				D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW }
			};
			D3D12_COMMAND_SIGNATURE_DESC signature{
				.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS),
				.NumArgumentDescs = static_cast<UINT>(argument_descs.size()),
				.pArgumentDescs = argument_descs.data(),
				.NodeMask = 0
			};

			HRESULT result = dev->CreateCommandSignature(&signature, nullptr, IID_PPV_ARGS(&cmd_sgn));
			ThrowIfFailed(result);

			return cmd_sgn;

			}();

		Microsoft::WRL::ComPtr<ID3D12CommandSignature> multidraw_indexed = [&dev]() {

			Microsoft::WRL::ComPtr<ID3D12CommandSignature> cmd_sgn;

			std::array argument_descs = {
				D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED }
			};
			D3D12_COMMAND_SIGNATURE_DESC signature{
				.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS),
				.NumArgumentDescs = static_cast<UINT>(argument_descs.size()),
				.pArgumentDescs = argument_descs.data(),
				.NodeMask = 0
			};

			HRESULT result = dev->CreateCommandSignature(&signature, nullptr, IID_PPV_ARGS(&cmd_sgn));
			ThrowIfFailed(result);

			return cmd_sgn;

			}();

		Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatch_indirect = [&dev]() {

			Microsoft::WRL::ComPtr<ID3D12CommandSignature> cmd_sgn;

			std::array argument_descs = {
				D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH }
			};
			D3D12_COMMAND_SIGNATURE_DESC signature{
				.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS),
				.NumArgumentDescs = static_cast<UINT>(argument_descs.size()),
				.pArgumentDescs = argument_descs.data(),
				.NodeMask = 0
			};

			HRESULT result = dev->CreateCommandSignature(&signature, nullptr, IID_PPV_ARGS(&cmd_sgn));
			ThrowIfFailed(result);

			return cmd_sgn;

			}();

		RegisterDevice(dev);

		return { 
			std::move(dev), 
			std::move(rm_tracker), 
			std::move(mem_alloc), 
			std::move(univ_alloc), 
			std::move(rtv_alloc), 
			std::move(dsv_alloc), 
			std::move(sampler_alloc),
			std::move(multidraw),
			std::move(multidraw_indexed),
			std::move(dispatch_indirect)
		};

	}

}
#endif // defined(_WIN32)