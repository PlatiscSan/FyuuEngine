module;
// Include <version> to detect standard library module support (__cpp_lib_modules).
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <shared_mutex>
#include <string>
#include <optional>
#include <variant>
#include <span>
#include <format>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include <D3D12MemAlloc.h>
module fyuu_rhi:d3d12_resource_impl;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :d3d12_resource;
import :types;
import :d3d12_logical_device;
import :d3d12_throw;
import :d3d12_types;

namespace {

	using namespace fyuu_rhi;
	using namespace fyuu_rhi::d3d12;

	D3D12_HEAP_TYPE VideoMemoryTypeToD3D12HeapType(VideoMemoryType type) noexcept {
		switch (type) {
		case fyuu_rhi::VideoMemoryType::DeviceReadback:
			return D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK;

		case fyuu_rhi::VideoMemoryType::HostVisible:
			return D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;

		case fyuu_rhi::VideoMemoryType::DeviceLocal:
		default:
			return D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		}
	}

	D3D12_RESOURCE_FLAGS DetermineD3D12ResourceFlags(ResourceFlags flag) noexcept {

		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
		if ((flag & ResourceFlags::StorageBuffer) != ResourceFlags::Unknown ||
			(flag & ResourceFlags::StorageTexelBuffer) != ResourceFlags::Unknown ||
			(flag & ResourceFlags::StorageTexture) != ResourceFlags::Unknown) {
			flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if ((flag & ResourceFlags::RenderTarget) != ResourceFlags::Unknown) {
			flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if ((flag & ResourceFlags::DepthStencil) != ResourceFlags::Unknown) {
			flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}
		return flags;

	}

	D3D12_RESOURCE_STATES DetermineInitialState(VideoMemoryType type) noexcept {
		switch (type) {
		case fyuu_rhi::VideoMemoryType::DeviceReadback:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		case fyuu_rhi::VideoMemoryType::HostVisible:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ;
		case fyuu_rhi::VideoMemoryType::DeviceLocal:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
		default:
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
		}
	}

	D3D12Resource::ViewType DetermineViewType(ResourceFlags flags) noexcept {
        if ((flags & ResourceFlags::DepthStencil) != ResourceFlags::Unknown) {
            return D3D12Resource::ViewType::DSV;
        }
        if ((flags & ResourceFlags::RenderTarget) != ResourceFlags::Unknown) {
            return D3D12Resource::ViewType::RTV;
        }
        if ((flags & ResourceFlags::StorageBuffer) != ResourceFlags::Unknown ||
            (flags & ResourceFlags::StorageTexelBuffer) != ResourceFlags::Unknown ||
            (flags & ResourceFlags::StorageTexture) != ResourceFlags::Unknown) {
            return D3D12Resource::ViewType::UAV;
        }
        if ((flags & ResourceFlags::UniformBuffer) != ResourceFlags::Unknown) {
            return D3D12Resource::ViewType::CBV;
        }

        return D3D12Resource::ViewType::SRV;
	}	

}

namespace fyuu_rhi::d3d12 {

	D3D12Resource::D3D12Resource(D3D12LogicalDevice const& logical_device, BufferSize buffer_size, VideoMemoryType type, ResourceFlags flags)
		: PolymorphicResourceBase(this),
		D3D12Common(this),
		m_logical_device_id(logical_device.GetID()),
		m_state(DetermineInitialState(type)),
		m_handle(
			[this, &logical_device, buffer_size, type, flags]() {
				
				bool is_buffer = (flags & ResourceFlags::Buffer) != ResourceFlags::Unknown;
				bool is_texture = (flags & ResourceFlags::Texture) != ResourceFlags::Unknown;

				if (is_texture) {
					throw std::runtime_error("trying to create buffer but texture flag is set");
				}

				if (!is_buffer) {
					throw std::runtime_error("trying to create buffer but no buffer flag is set");
				}

				Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation;

				auto allocator = logical_device.GetVideoMemoryAllocator();

				D3D12MA::ALLOCATION_DESC alloc_desc{
					D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_STRATEGY_FIRST_FIT,
					VideoMemoryTypeToD3D12HeapType(type),
					D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS
				};

				auto resource_desc = CD3DX12_RESOURCE_DESC::Buffer(buffer_size, DetermineD3D12ResourceFlags(flags));

				ThrowIfFailed(
					allocator->CreateResource(
						&alloc_desc,
						&resource_desc,
						m_state,
						nullptr,
						&allocation,
						__uuidof(ID3D12Resource2),
						nullptr
					)
				);

				return allocation;

			}()),
		m_view(DetermineViewType(flags)),
		m_type(ResourceType::Buffer) {
	}

	D3D12Resource::D3D12Resource(D3D12LogicalDevice const& logical_device, TextureSize const& texture_size, VideoMemoryType type, ResourceFlags flags)
		: PolymorphicResourceBase(this),
		D3D12Common(this),
		m_logical_device_id(logical_device.GetID()),
		m_state(DetermineInitialState(type)), 
		m_handle(
			[this, &logical_device, &texture_size, type, flags]() {
				bool is_buffer = (flags & ResourceFlags::Buffer) != ResourceFlags::Unknown;
				bool is_texture = (flags & ResourceFlags::Texture) != ResourceFlags::Unknown;

				if (is_buffer) {
					throw std::runtime_error("trying to create texture but buffer flag is set");
				}

				if (!is_texture) {
					throw std::runtime_error("trying to create texture but no texture flag is set");
				}

				DXGI_FORMAT dx_format = DetermineFormat(flags);
				std::optional<CD3DX12_RESOURCE_DESC> resource_desc;

				if ((flags & ResourceFlags::Texture1D) != ResourceFlags::Unknown) {
					resource_desc = CD3DX12_RESOURCE_DESC::Tex1D(dx_format, texture_size.width);
				}

				if ((flags & ResourceFlags::Texture2D) != ResourceFlags::Unknown) {
					resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(dx_format, texture_size.width, static_cast<UINT>(texture_size.height));
				}

				if ((flags & ResourceFlags::Texture3D) != ResourceFlags::Unknown) {
					resource_desc = CD3DX12_RESOURCE_DESC::Tex3D(dx_format, texture_size.width, static_cast<UINT>(texture_size.height), static_cast<UINT16>(texture_size.depth));
				}

				if ((flags & ResourceFlags::TextureCube) != ResourceFlags::Unknown) {
					resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(dx_format, texture_size.width, static_cast<UINT>(texture_size.height), 6u);
				}

				if (!resource_desc) {
					throw std::runtime_error("trying to create texture but no texture dimension flag is set");
				}

				resource_desc->Flags = DetermineD3D12ResourceFlags(flags);

				Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation;

				auto allocator = logical_device.GetVideoMemoryAllocator();

				D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;

				if ((flags & ResourceFlags::RenderTarget) != ResourceFlags::Unknown ||
					(flags & ResourceFlags::DepthStencil) != ResourceFlags::Unknown) {
					heap_flags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
				}

				D3D12MA::ALLOCATION_DESC alloc_desc{
					D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_STRATEGY_FIRST_FIT,
					VideoMemoryTypeToD3D12HeapType(type),
					heap_flags
				};

				ThrowIfFailed(
					allocator->CreateResource(
						&alloc_desc,
						&*resource_desc,
						m_state,
						nullptr,
						&allocation,
						__uuidof(ID3D12Resource2),
						nullptr
					)
				);

				return allocation;

			}()),
		m_view(DetermineViewType(flags)),
		m_type(ResourceType::Texture) {
	}

	D3D12Resource::D3D12Resource(D3D12LogicalDevice const& logical_device, IDXGISwapChain4* swap_chain, std::size_t index)
		: PolymorphicResourceBase(this),
		D3D12Common(this),
		m_logical_device_id(logical_device.GetID()),
		m_state(D3D12_RESOURCE_STATE_PRESENT),
		m_handle(
			[swap_chain, index]() {
				Microsoft::WRL::ComPtr<ID3D12Resource2> back_buffer;
				ThrowIfFailed(swap_chain->GetBuffer(index, IID_PPV_ARGS(&back_buffer)));
				std::wstring buffer_name = std::format(L"Swap chain back buffer {}", index);
				ThrowIfFailed(back_buffer->SetName(buffer_name.data()));
				return back_buffer;
			}()),
		m_view(ViewType::RTV),
		m_type(ResourceType::Texture) {
	}

	D3D12_RESOURCE_DESC D3D12Resource::GetDescription() const noexcept {

		auto overload = plastic::utility::Overload{
			[](std::monostate) -> D3D12_RESOURCE_DESC { return {}; },
			[](Microsoft::WRL::ComPtr<D3D12MA::Allocation> const& allocation) {
				return allocation->GetResource()->GetDesc();
			},
			[](Microsoft::WRL::ComPtr<ID3D12Resource2> const& resource) {
				return resource->GetDesc();
			}
		};

		return std::visit(overload, m_handle);

    }

	D3D12_RESOURCE_DESC1 D3D12Resource::GetDescription1() const noexcept {
		auto overload = plastic::utility::Overload{
			[](std::monostate) -> D3D12_RESOURCE_DESC1 { return {}; },
			[](Microsoft::WRL::ComPtr<D3D12MA::Allocation> const& allocation) {
				Microsoft::WRL::ComPtr<ID3D12Resource2> resource2;
				ThrowIfFailed(allocation->GetResource()->QueryInterface(IID_PPV_ARGS(&resource2)));
				return resource2->GetDesc1();
			},
			[](Microsoft::WRL::ComPtr<ID3D12Resource2> const& resource) {
				return resource->GetDesc1();
			}
		};

		return std::visit(overload, m_handle);
	}

	ID3D12Resource2* D3D12Resource::GetRawNative() const noexcept {

		auto overload = plastic::utility::Overload{
			[](std::monostate) -> ID3D12Resource2* { return nullptr; },
			[](Microsoft::WRL::ComPtr<D3D12MA::Allocation> const& allocation) {
				return static_cast<ID3D12Resource2*>(allocation->GetResource());
			},
			[](Microsoft::WRL::ComPtr<ID3D12Resource2> const& resource) {
				return resource.Get();
			}			
		};

		return std::visit(overload, m_handle);
	}

    D3D12_RESOURCE_STATES D3D12Resource::GetState() const {
		return m_state;
	}

	void D3D12Resource::SetState(D3D12_RESOURCE_STATES state) noexcept {
		m_state = state;
	}

	D3D12Resource::ViewType D3D12Resource::GetViewType() const noexcept {
		return m_view;
	}

	D3D12Resource::ResourceType D3D12Resource::GetResourceType() const noexcept {
		return m_type;
	}

	std::size_t D3D12Resource::QuerySupportedMultiSampleQuality(std::size_t sample_count) const {

       	if (m_logical_device_id) {
			throw std::invalid_argument("invalid logical device");
		}
		auto* logical_device = plastic::utility::QueryObject<D3D12LogicalDevice>(*m_logical_device_id);
		if (!logical_device) {
			throw std::runtime_error("logical device lost");
		}

        D3D12_RESOURCE_DESC desc = GetDescription();

        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS ms_quality_levels = {};
        ms_quality_levels.Format = desc.Format;
        ms_quality_levels.SampleCount = static_cast<UINT>(sample_count);
        ms_quality_levels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
        ms_quality_levels.NumQualityLevels = 0;
        
        HRESULT hr = logical_device->GetRawNative()->CheckFeatureSupport(
            D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
            &ms_quality_levels,
            sizeof(ms_quality_levels)
        );
        
        if (SUCCEEDED(hr) && ms_quality_levels.NumQualityLevels > 0u) {
            UINT supported_quality_count = ms_quality_levels.NumQualityLevels;
            return supported_quality_count - 1u; 
        } 
		else {
            return 0u;
        }
	}
}

#endif // defined(_WIN32)