module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <array>
#include <string_view>
#include <source_location>
#include <format>
#endif // !defined(__cpp_lib_modules)
#if defined(_WIN32)
#include <D3D12MemAlloc.h>
#include <directx/d3dx12.h>
#include <wrl.h>
#include <comdef.h>
#endif // defined(_WIN32)
#define BOOST_DISABLE_ASSERTS
#include <boost/locale.hpp>
module fyuu_rhi:d3d12_logical_dev;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :d3d12_traits;
import :d3d12_utility;
import :command_types;
import :resource_types;
import :d3d12_resource_tracker;
import :d3d12_cache;

namespace {

	using namespace fyuu_rhi;

	D3D12MA::ALLOCATION_FLAGS ExtractAllocationFlags(ResourceFlags const& flags) {

		D3D12MA::ALLOCATION_FLAGS d3d12ma_flags{};

		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::UndedicatedAllocation, ResourceFlagBits::DedicatedAllocation);
		if (is_conflicting) {
			throw std::invalid_argument("UndedicatedAllocation and DedicatedAllocation are set simultaneously");
		}
		
		if (flags.Test(ResourceFlagBits::UndedicatedAllocation)) {
			d3d12ma_flags |= D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_NEVER_ALLOCATE;
		}
		else if (flags.Test(ResourceFlagBits::DedicatedAllocation)) {
			d3d12ma_flags |= D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_COMMITTED;
		}
		else {

		}

		if (flags.Test(ResourceFlagBits::AllocationWithinBudget)) {
			d3d12ma_flags |= D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_WITHIN_BUDGET;
		}
		if (flags.Test(ResourceFlagBits::AllocationAtUpperAddress)) {
			d3d12ma_flags |= D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_UPPER_ADDRESS;
		}
		if (flags.Test(ResourceFlagBits::AllocationAliasAllowed)) {
			d3d12ma_flags |= D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_CAN_ALIAS;
		}

		is_conflicting = flags.TestSingleInRange(ResourceFlagBits::MinOffsetAllocation, ResourceFlagBits::FirstFitAllocation);
		if (is_conflicting) {
			throw std::invalid_argument("MinOffsetAllocation BestFitAllocation or FirstFitAllocation are set simultaneously");
		}
		else if (flags.Test(ResourceFlagBits::MinOffsetAllocation)) {
			d3d12ma_flags |= D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_STRATEGY_MIN_OFFSET;
		}
		else if (flags.Test(ResourceFlagBits::BestFitAllocation)) {
			d3d12ma_flags |= D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_STRATEGY_BEST_FIT;
		}
		else if (flags.Test(ResourceFlagBits::FirstFitAllocation)) {
			d3d12ma_flags |= D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_STRATEGY_FIRST_FIT;
		}
		else {

		}

		return d3d12ma_flags;

	}

	D3D12_HEAP_TYPE ExtractHeapType(ResourceFlags const& flags) {
		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::DeviceLocal, ResourceFlagBits::DeviceReadback);
		if (is_conflicting) {
			throw std::invalid_argument("DeviceLocal HostVisible or DeviceReadback are set simultaneously");
		}

		if (flags.Test(ResourceFlagBits::DeviceLocal)) {
			return D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		}
		else if (flags.Test(ResourceFlagBits::HostVisible)) {
			return D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
		}
		else if (flags.Test(ResourceFlagBits::DeviceReadback)) {
			return D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK;
		} 
		else {
			return D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		}

	}

	D3D12_RESOURCE_FLAGS ExtractResourceFlags(ResourceFlags const& flags) noexcept {
		
		D3D12_RESOURCE_FLAGS res_flags = D3D12_RESOURCE_FLAG_NONE;

		if (flags.Test(ResourceFlagBits::StorageTexelBuffer) ||
			flags.Test(ResourceFlagBits::StorageBuffer) ||
			flags.Test(ResourceFlagBits::StorageBinding) ||
			flags.Test(ResourceFlagBits::StorageAttachment)) {
			res_flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		if (flags.Test(ResourceFlagBits::RenderAttachment)) {
			bool is_ds =
				flags.Test(ResourceFlagBits::D16Unorm) ||
				flags.Test(ResourceFlagBits::D24UnormS8Uint) ||
				flags.Test(ResourceFlagBits::D32Float) ||
				flags.Test(ResourceFlagBits::D32FloatS8X24Uint);

			if (is_ds) {
				res_flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			}
			else {
				res_flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			}
		}

		return res_flags;

	}

	D3D12_RESOURCE_STATES DetermineInitialState(ResourceFlags const& flags) {
		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::DeviceLocal, ResourceFlagBits::DeviceReadback);
		if (is_conflicting) {
			throw std::invalid_argument("DeviceLocal HostVisible or DeviceReadback are set simultaneously");
		}
		if (flags.Test(ResourceFlagBits::DeviceLocal)) {
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
		}
		else if (flags.Test(ResourceFlagBits::HostVisible)) {
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		else if (flags.Test(ResourceFlagBits::DeviceReadback)) {
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		}
		else {
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
		}
	}

	DXGI_FORMAT ExtractFormat(ResourceFlags const& flags) {

		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::R8Unorm, ResourceFlagBits::Bc7UnormSrgb);
		if (is_conflicting) {
			throw std::invalid_argument("Only one format can be set");
		}

		// 8‑bit per component (1‑channel)
		if (flags.Test(ResourceFlagBits::R8Unorm)) return DXGI_FORMAT_R8_UNORM;
		else if (flags.Test(ResourceFlagBits::R8Snorm)) return DXGI_FORMAT_R8_SNORM;
		else if (flags.Test(ResourceFlagBits::R8Uint)) return DXGI_FORMAT_R8_UINT;
		else if (flags.Test(ResourceFlagBits::R8Sint)) return DXGI_FORMAT_R8_SINT;
	
		// 8‑bit per component (2‑channel)
		else if (flags.Test(ResourceFlagBits::R8G8Unorm)) return DXGI_FORMAT_R8G8_UNORM;
		else if (flags.Test(ResourceFlagBits::R8G8Snorm)) return DXGI_FORMAT_R8G8_SNORM;
		else if (flags.Test(ResourceFlagBits::R8G8Uint)) return DXGI_FORMAT_R8G8_UINT;
		else if (flags.Test(ResourceFlagBits::R8G8Sint)) return DXGI_FORMAT_R8G8_SINT;
	
		// 8‑bit per component (4‑channel)
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Unorm)) return DXGI_FORMAT_R8G8B8A8_UNORM;
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Snorm)) return DXGI_FORMAT_R8G8B8A8_SNORM;
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Uint)) return DXGI_FORMAT_R8G8B8A8_UINT;
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Sint)) return DXGI_FORMAT_R8G8B8A8_SINT;
		else if (flags.Test(ResourceFlagBits::R8G8B8A8Srgb)) return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		else if (flags.Test(ResourceFlagBits::B8G8R8A8Srgb)) return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	
		// 16‑bit per component (1‑channel)
		else if (flags.Test(ResourceFlagBits::R16Unorm)) return DXGI_FORMAT_R16_UNORM;
		else if (flags.Test(ResourceFlagBits::R16Snorm)) return DXGI_FORMAT_R16_SNORM;
		else if (flags.Test(ResourceFlagBits::R16Uint)) return DXGI_FORMAT_R16_UINT;
		else if (flags.Test(ResourceFlagBits::R16Sint)) return DXGI_FORMAT_R16_SINT;
		else if (flags.Test(ResourceFlagBits::R16Float)) return DXGI_FORMAT_R16_FLOAT;
	
		// 16‑bit per component (2‑channel)
		else if (flags.Test(ResourceFlagBits::R16G16Unorm)) return DXGI_FORMAT_R16G16_UNORM;
		else if (flags.Test(ResourceFlagBits::R16G16Snorm)) return DXGI_FORMAT_R16G16_SNORM;
		else if (flags.Test(ResourceFlagBits::R16G16Uint)) return DXGI_FORMAT_R16G16_UINT;
		else if (flags.Test(ResourceFlagBits::R16G16Sint)) return DXGI_FORMAT_R16G16_SINT;
		else if (flags.Test(ResourceFlagBits::R16G16Float)) return DXGI_FORMAT_R16G16_FLOAT;
	
		// 16‑bit per component (4‑channel)
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Unorm)) return DXGI_FORMAT_R16G16B16A16_UNORM;
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Snorm)) return DXGI_FORMAT_R16G16B16A16_SNORM;
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Uint)) return DXGI_FORMAT_R16G16B16A16_UINT;
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Sint)) return DXGI_FORMAT_R16G16B16A16_SINT;
		else if (flags.Test(ResourceFlagBits::R16G16B16A16Float)) return DXGI_FORMAT_R16G16B16A16_FLOAT;
	
		// 32‑bit per component (1‑channel)
		else if (flags.Test(ResourceFlagBits::R32Uint)) return DXGI_FORMAT_R32_UINT;
		else if (flags.Test(ResourceFlagBits::R32Sint)) return DXGI_FORMAT_R32_SINT;
		else if (flags.Test(ResourceFlagBits::R32Float)) return DXGI_FORMAT_R32_FLOAT;
	
		// 32‑bit per component (2‑channel)
		else if (flags.Test(ResourceFlagBits::R32G32Uint)) return DXGI_FORMAT_R32G32_UINT;
		else if (flags.Test(ResourceFlagBits::R32G32Sint)) return DXGI_FORMAT_R32G32_SINT;
		else if (flags.Test(ResourceFlagBits::R32G32Float)) return DXGI_FORMAT_R32G32_FLOAT;
	
		// 32‑bit per component (4‑channel)
		else if (flags.Test(ResourceFlagBits::R32G32B32A32Uint)) return DXGI_FORMAT_R32G32B32A32_UINT;
		else if (flags.Test(ResourceFlagBits::R32G32B32A32Sint)) return DXGI_FORMAT_R32G32B32A32_SINT;
		else if (flags.Test(ResourceFlagBits::R32G32B32A32Float)) return DXGI_FORMAT_R32G32B32A32_FLOAT;
	
		// Packed formats (note: DXGI uses BGR ordering for 5/6/5 and 5/5/5/1)
		else if (flags.Test(ResourceFlagBits::R10G10B10A2Unorm)) return DXGI_FORMAT_R10G10B10A2_UNORM;
		else if (flags.Test(ResourceFlagBits::R10G10B10A2Uint)) return DXGI_FORMAT_R10G10B10A2_UINT;
		else if (flags.Test(ResourceFlagBits::R11G11B10Float)) return DXGI_FORMAT_R11G11B10_FLOAT;
		else if (flags.Test(ResourceFlagBits::R9G9B9E5SharedExp)) return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
	
		// Depth/stencil
		else if (flags.Test(ResourceFlagBits::D16Unorm)) return DXGI_FORMAT_D16_UNORM;
		else if (flags.Test(ResourceFlagBits::D24UnormS8Uint)) return DXGI_FORMAT_D24_UNORM_S8_UINT;
		else if (flags.Test(ResourceFlagBits::D32Float)) return DXGI_FORMAT_D32_FLOAT;
		else if (flags.Test(ResourceFlagBits::D32FloatS8X24Uint)) return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	
		// BC compressed formats
		else if (flags.Test(ResourceFlagBits::Bc1Unorm)) return DXGI_FORMAT_BC1_UNORM;
		else if (flags.Test(ResourceFlagBits::Bc1UnormSrgb)) return DXGI_FORMAT_BC1_UNORM_SRGB;
		else if (flags.Test(ResourceFlagBits::Bc2Unorm)) return DXGI_FORMAT_BC2_UNORM;
		else if (flags.Test(ResourceFlagBits::Bc2UnormSrgb)) return DXGI_FORMAT_BC2_UNORM_SRGB;
		else if (flags.Test(ResourceFlagBits::Bc3Unorm)) return DXGI_FORMAT_BC3_UNORM;
		else if (flags.Test(ResourceFlagBits::Bc3UnormSrgb)) return DXGI_FORMAT_BC3_UNORM_SRGB;
		else if (flags.Test(ResourceFlagBits::Bc4Unorm)) return DXGI_FORMAT_BC4_UNORM;
		else if (flags.Test(ResourceFlagBits::Bc4Snorm)) return DXGI_FORMAT_BC4_SNORM;
		else if (flags.Test(ResourceFlagBits::Bc5Unorm)) return DXGI_FORMAT_BC5_UNORM;
		else if (flags.Test(ResourceFlagBits::Bc5Snorm)) return DXGI_FORMAT_BC5_SNORM;
		else if (flags.Test(ResourceFlagBits::Bc6HUfloat)) return DXGI_FORMAT_BC6H_UF16;
		else if (flags.Test(ResourceFlagBits::Bc6HSfloat)) return DXGI_FORMAT_BC6H_SF16;
		else if (flags.Test(ResourceFlagBits::Bc7Unorm)) return DXGI_FORMAT_BC7_UNORM;
		else if (flags.Test(ResourceFlagBits::Bc7UnormSrgb)) return DXGI_FORMAT_BC7_UNORM_SRGB;

		else return DXGI_FORMAT_UNKNOWN;

	}

	UINT ExtractSampleCount(ResourceFlags const& flags) {
		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::Sample1, ResourceFlagBits::Sample64);
		if (is_conflicting) {
			throw std::invalid_argument("Only sample count can be set");
		}
		else if (flags.Test(ResourceFlagBits::Sample1)) {
			return 1u;
		}
		else if (flags.Test(ResourceFlagBits::Sample2)) {
			return 2u;
		}
		else if (flags.Test(ResourceFlagBits::Sample4)) {
			return 4u;
		}
		else if (flags.Test(ResourceFlagBits::Sample8)) {
			return 8u;
		}
		else if (flags.Test(ResourceFlagBits::Sample16)) {
			return 16u;
		}
		else if (flags.Test(ResourceFlagBits::Sample32)) {
			return 32u;
		}
		else if (flags.Test(ResourceFlagBits::Sample64)) {
			return 64u;
		}
		else {
			return 1u;
		}

	}

	D3D12_TEXTURE_LAYOUT ExtractTextureLayout(ResourceFlags const& flags) {
		bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::DeviceLocal, ResourceFlagBits::DeviceReadback);
		if (is_conflicting) {
			throw std::invalid_argument("DeviceLocal HostVisible or DeviceReadback are set simultaneously");
		}
		else if (flags.Test(ResourceFlagBits::DeviceLocal)) {
			return D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		}
		else if (flags.Test(ResourceFlagBits::HostVisible)) {
			return D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		}
		else if (flags.Test(ResourceFlagBits::DeviceReadback)) {
			return D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		}
		else {
			return D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;			
		}
	}

}

namespace fyuu_rhi::d3d12 {

	Backend::Promise Backend::CreatePromise(Backend::LogicalDevice const& ld) {
		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		HRESULT result = ld.impl->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		ThrowIfFailed(result);
		return { std::move(fence), TimelineCounter{}, {} };
	}

	Microsoft::WRL::ComPtr<D3D12MA::Allocation> Backend::CreateBuffer(Backend::LogicalDevice const& ld, std::size_t size_in_bytes, ResourceFlags const& flags) {
		D3D12MA::ALLOCATION_DESC alloc_desc{
			D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_STRATEGY_FIRST_FIT,
			ExtractHeapType(flags),
			D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS,
			nullptr,
			ld.impl.Get()
		};
		auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(size_in_bytes, ExtractResourceFlags(flags));
		auto init_state = DetermineInitialState(flags);
		Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation;
		Microsoft::WRL::ComPtr<ID3D12Resource> res;
		HRESULT result = ld.mem_alloc->CreateResource(
			&alloc_desc,
			&res_desc,
			init_state,
			nullptr,
			&allocation,
			IID_PPV_ARGS(&res)
		);
		ThrowIfFailed(result);
		RegisterResource(res, init_state);
		return allocation;
	}

	Microsoft::WRL::ComPtr<D3D12MA::Allocation> Backend::CreateTexture(Backend::LogicalDevice const& ld, std::size_t width, std::size_t height, std::size_t depth_arr_layers, std::size_t mip_lvl_cnt, ResourceFlags const& flags) {
		D3D12MA::ALLOCATION_DESC alloc_desc{
			D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_STRATEGY_FIRST_FIT,
			ExtractHeapType(flags),
			D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS,
			nullptr,
			nullptr
		};
		DXGI_FORMAT format = ExtractFormat(flags);
		UINT sample_cnt = ExtractSampleCount(flags);
		D3D12_RESOURCE_FLAGS res_flags = ExtractResourceFlags(flags);
		D3D12_TEXTURE_LAYOUT tex_layout = ExtractTextureLayout(flags);
		auto res_desc = [&]() {
			bool is_conflicting = flags.TestSingleInRange(ResourceFlagBits::Texture1D, ResourceFlagBits::Texture3D);
			if (is_conflicting) {
				throw std::invalid_argument("Texture1D Texture2D or Texture3D are set simultaneously");
			}
			if (flags.Test(ResourceFlagBits::Texture1D)) {
				return CD3DX12_RESOURCE_DESC::Tex1D(
					format, width, static_cast<UINT16>(depth_arr_layers), 
					static_cast<UINT16>(mip_lvl_cnt), res_flags, tex_layout
				);
			}
			if (flags.Test(ResourceFlagBits::Texture2D)) {
				return CD3DX12_RESOURCE_DESC::Tex2D(
					format, width, static_cast<UINT>(height), static_cast<UINT16>(depth_arr_layers), 
					static_cast<UINT16>(mip_lvl_cnt), sample_cnt,
					QueryQualityLevels(ld.impl.Get(), format, sample_cnt), 
					res_flags, tex_layout
				);
			}
			if (flags.Test(ResourceFlagBits::Texture3D)) {
				return CD3DX12_RESOURCE_DESC::Tex3D(
					format, width, static_cast<UINT>(height), static_cast<UINT16>(depth_arr_layers), 
					static_cast<UINT16>(mip_lvl_cnt), res_flags, tex_layout
				);
			}
			return CD3DX12_RESOURCE_DESC::Tex2D(
				format, width, static_cast<UINT>(height), static_cast<UINT16>(depth_arr_layers), 
				static_cast<UINT16>(mip_lvl_cnt), sample_cnt,
				QueryQualityLevels(ld.impl.Get(), format, sample_cnt), 
				res_flags, tex_layout
			);
			}();
		auto init_state = DetermineInitialState(flags);
		Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation;
		Microsoft::WRL::ComPtr<ID3D12Resource> res;
		HRESULT result = ld.mem_alloc->CreateResource(
			&alloc_desc,
			&res_desc,
			init_state,
			nullptr,
			&allocation,
			IID_PPV_ARGS(&res)
		);
		ThrowIfFailed(result);
		RegisterResource(res, init_state);
		return allocation;
	}
	
	std::shared_ptr<D3D12DescriptorHandle> Backend::CreateTextureView(Backend::LogicalDevice const& ld, Microsoft::WRL::ComPtr<D3D12MA::Allocation> const& alloc, std::size_t base_mip_lvl, std::size_t mip_lvl_cnt, std::size_t base_arr_layer, std::size_t arr_layer_cnt, ResourceFlags const& flags) {
		
	}

}
#endif // defined(_WIN32)