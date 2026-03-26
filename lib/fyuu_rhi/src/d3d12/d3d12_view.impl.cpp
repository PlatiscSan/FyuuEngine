module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <utility>
#include <variant>
#include <type_traits>
#include <concepts>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include <D3D12MemAlloc.h>
module fyuu_rhi:d3d12_view_impl;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :d3d12_view;
import :types;
import :d3d12_descriptor_allocator;
import :d3d12_resource;
import :d3d12_logical_device;
import :d3d12_descriptor_allocator;

namespace fyuu_rhi::d3d12 {

	D3D12View::D3D12View(D3D12LogicalDevice const& logical_device, D3D12Resource const& resource, BufferSize buffer_size, BufferSize offset)
		: PolymorphicViewBase(this),
		D3D12Common(this),
		m_handle(
			[&logical_device, &resource, buffer_size, offset]() {

				if (resource.GetResourceType() != D3D12Resource::ResourceType::Buffer) {
					throw std::invalid_argument("Resource is not a buffer");
				}
  
				auto view_type = resource.GetViewType();
				D3D12_RESOURCE_DESC desc = resource.GetDescription();
				ID3D12Resource2* raw_resource = resource.GetRawNative();

				DXGI_FORMAT format = desc.Format;
				if (format == DXGI_FORMAT_UNKNOWN) {
					throw std::invalid_argument("Only formatted buffers (texel buffers) are supported. ");
				}
  
				UINT element_size = static_cast<UINT>(DXGIFormatSize(format));
				if (element_size == 0) {
					throw std::invalid_argument("Unsupported format for buffer view");
				}
  
				Handle handle;
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc{};
				D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
				D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
				
				switch (view_type) {
				case D3D12Resource::ViewType::CBV: 
					cbv_desc.BufferLocation = raw_resource->GetGPUVirtualAddress() + offset;
					cbv_desc.SizeInBytes = static_cast<UINT>(buffer_size);
					logical_device.CreateCBV(
						handle.emplace<D3D12UniversalViewAllocator::Allocation>(
							logical_device.GetUniversalViewAllocator()->Allocate()
						)->cpu_handle,
						cbv_desc
					);
					break;
			
				case D3D12Resource::ViewType::UAV:
					uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
					uav_desc.Format = format;
					uav_desc.Buffer.FirstElement = static_cast<UINT>(offset / element_size);
					uav_desc.Buffer.NumElements = static_cast<UINT>(buffer_size / element_size);
					uav_desc.Buffer.StructureByteStride = 0;
					uav_desc.Buffer.CounterOffsetInBytes = 0;
					uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
					logical_device.CreateUAV(
						raw_resource, 
						handle.emplace<D3D12UniversalViewAllocator::Allocation>(
							logical_device.GetUniversalViewAllocator()->Allocate()
						)->cpu_handle, 
						uav_desc
					);
					break;
				
				case D3D12Resource::ViewType::SRV:
					srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
					srv_desc.Format = format;
					srv_desc.Buffer.FirstElement = static_cast<UINT>(offset / element_size);
					srv_desc.Buffer.NumElements = static_cast<UINT>(buffer_size / element_size);
					srv_desc.Buffer.StructureByteStride = 0;
					srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
					logical_device.CreateSRV(
						raw_resource, 
						handle.emplace<D3D12UniversalViewAllocator::Allocation>(
							logical_device.GetUniversalViewAllocator()->Allocate()
						)->cpu_handle, 
						srv_desc
					);
					break;
				
				default:
					throw std::invalid_argument("Buffer view type not supported");
				}
  
				return handle;
				
			}()) {
	}

	D3D12View::D3D12View(D3D12LogicalDevice const& logical_device, D3D12Resource const& resource, std::size_t base_mip_level, std::size_t level_count, std::size_t base_layer, std::size_t layer_count)
		: PolymorphicViewBase(this),
		D3D12Common(this),
		m_handle(
			[&logical_device, &resource, base_mip_level, level_count, base_layer, layer_count]() {

				if (resource.GetResourceType() != D3D12Resource::ResourceType::Texture) {
					throw std::invalid_argument("D3D12View: resource is not a texture");
				}
  
				auto view_type = resource.GetViewType();
				D3D12_RESOURCE_DESC desc = resource.GetDescription();
				ID3D12Resource2* raw_resource = resource.GetRawNative();
  
				if (base_mip_level >= desc.MipLevels) {
					throw std::out_of_range("base_mip_level out of range");
				}
				if (level_count == 0 || base_mip_level + level_count > desc.MipLevels) {
					throw std::out_of_range("level_count out of range");
				}
				UINT array_size = desc.DepthOrArraySize == 1 ? 1 : desc.DepthOrArraySize;
				if (base_layer >= array_size) {
					throw std::out_of_range("base_layer out of range");
				}
				if (layer_count == 0 || base_layer + layer_count > array_size) {
					throw std::out_of_range("layer_count out of range");
				}
  
				D3D12_SRV_DIMENSION srv_dim = D3D12_SRV_DIMENSION_UNKNOWN;
				D3D12_RTV_DIMENSION rtv_dim = D3D12_RTV_DIMENSION_UNKNOWN;
				D3D12_DSV_DIMENSION dsv_dim = D3D12_DSV_DIMENSION_UNKNOWN;
  
				bool is_cube = false;

				if (desc.DepthOrArraySize >= 6 && (desc.DepthOrArraySize % 6 == 0)) {
					is_cube = true;
				}
  
				switch (desc.Dimension) {
				case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
					if (array_size > 1) {
						srv_dim = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
						rtv_dim = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
						dsv_dim = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
					} else {
						srv_dim = D3D12_SRV_DIMENSION_TEXTURE1D;
						rtv_dim = D3D12_RTV_DIMENSION_TEXTURE1D;
						dsv_dim = D3D12_DSV_DIMENSION_TEXTURE1D;
					}
					break;
				case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
					if (is_cube) {
						if (array_size > 6) {
							srv_dim = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
							rtv_dim = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
							dsv_dim = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
						} else {
							srv_dim = D3D12_SRV_DIMENSION_TEXTURECUBE;
							rtv_dim = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
							dsv_dim = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
						}
					} else {
						if (array_size > 1) {
							srv_dim = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
							rtv_dim = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
							dsv_dim = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
						} else {
							srv_dim = D3D12_SRV_DIMENSION_TEXTURE2D;
							rtv_dim = D3D12_RTV_DIMENSION_TEXTURE2D;
							dsv_dim = D3D12_DSV_DIMENSION_TEXTURE2D;
						}
					}
					break;
				case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
					srv_dim = D3D12_SRV_DIMENSION_TEXTURE3D;
					rtv_dim = D3D12_RTV_DIMENSION_TEXTURE3D;
					dsv_dim = D3D12_DSV_DIMENSION_UNKNOWN;
					break;
				default:
					throw std::invalid_argument("Unsupported texture dimension");
				}
  
				Handle handle;
				D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
				D3D12_RENDER_TARGET_VIEW_DESC rtv_desc{};
				D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};

				switch (view_type) {
				case D3D12Resource::ViewType::SRV:
					srv_desc.Format = desc.Format;
					srv_desc.ViewDimension = srv_dim;
					srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  
					switch (srv_dim) {
					case D3D12_SRV_DIMENSION_TEXTURE1D:
						srv_desc.Texture1D.MostDetailedMip = static_cast<UINT>(base_mip_level);
						srv_desc.Texture1D.MipLevels = static_cast<UINT>(level_count);
						break;
					case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
						srv_desc.Texture1DArray.MostDetailedMip = static_cast<UINT>(base_mip_level);
						srv_desc.Texture1DArray.MipLevels = static_cast<UINT>(level_count);
						srv_desc.Texture1DArray.FirstArraySlice = static_cast<UINT>(base_layer);
						srv_desc.Texture1DArray.ArraySize = static_cast<UINT>(layer_count);
						break;
					case D3D12_SRV_DIMENSION_TEXTURE2D:
						srv_desc.Texture2D.MostDetailedMip = static_cast<UINT>(base_mip_level);
						srv_desc.Texture2D.MipLevels = static_cast<UINT>(level_count);
						break;
					case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
						srv_desc.Texture2DArray.MostDetailedMip = static_cast<UINT>(base_mip_level);
						srv_desc.Texture2DArray.MipLevels = static_cast<UINT>(level_count);
						srv_desc.Texture2DArray.FirstArraySlice = static_cast<UINT>(base_layer);
						srv_desc.Texture2DArray.ArraySize = static_cast<UINT>(layer_count);
						break;
					case D3D12_SRV_DIMENSION_TEXTURE3D:
						srv_desc.Texture3D.MostDetailedMip = static_cast<UINT>(base_mip_level);
						srv_desc.Texture3D.MipLevels = static_cast<UINT>(level_count);
						break;
					case D3D12_SRV_DIMENSION_TEXTURECUBE:
						srv_desc.TextureCube.MostDetailedMip = static_cast<UINT>(base_mip_level);
						srv_desc.TextureCube.MipLevels = static_cast<UINT>(level_count);
						break;
					case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
						srv_desc.TextureCubeArray.MostDetailedMip = static_cast<UINT>(base_mip_level);
						srv_desc.TextureCubeArray.MipLevels = static_cast<UINT>(level_count);
						srv_desc.TextureCubeArray.First2DArrayFace = static_cast<UINT>(base_layer);
						srv_desc.TextureCubeArray.NumCubes = static_cast<UINT>(layer_count / 6);
						break;
					default:
						throw std::invalid_argument("Unsupported SRV dimension");
					}
  
					logical_device.CreateSRV(
						raw_resource, 
						handle.emplace<D3D12UniversalViewAllocator::Allocation>(
							logical_device.GetUniversalViewAllocator()->Allocate()
						)->cpu_handle, 
						srv_desc
					);

					break;
				
				case D3D12Resource::ViewType::RTV:
					rtv_desc.Format = desc.Format;
					rtv_desc.ViewDimension = rtv_dim;
  
					switch (rtv_dim) {
					case D3D12_RTV_DIMENSION_TEXTURE1D:
						rtv_desc.Texture1D.MipSlice = static_cast<UINT>(base_mip_level);
						break;
					case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
						rtv_desc.Texture1DArray.MipSlice = static_cast<UINT>(base_mip_level);
						rtv_desc.Texture1DArray.FirstArraySlice = static_cast<UINT>(base_layer);
						rtv_desc.Texture1DArray.ArraySize = static_cast<UINT>(layer_count);
						break;
					case D3D12_RTV_DIMENSION_TEXTURE2D:
						rtv_desc.Texture2D.MipSlice = static_cast<UINT>(base_mip_level);
						break;
					case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
						rtv_desc.Texture2DArray.MipSlice = static_cast<UINT>(base_mip_level);
						rtv_desc.Texture2DArray.FirstArraySlice = static_cast<UINT>(base_layer);
						rtv_desc.Texture2DArray.ArraySize = static_cast<UINT>(layer_count);
						break;
					case D3D12_RTV_DIMENSION_TEXTURE3D:
						rtv_desc.Texture3D.MipSlice = static_cast<UINT>(base_mip_level);
						rtv_desc.Texture3D.FirstWSlice = static_cast<UINT>(base_layer);
						rtv_desc.Texture3D.WSize = static_cast<UINT>(layer_count);
						break;
					default:
						throw std::invalid_argument("Unsupported RTV dimension");
					}

					logical_device.CreateRTV(
						raw_resource, 
						handle.emplace<D3D12RTVAllocator::Allocation>(
							logical_device.GetRenderTargetViewAllocator()->Allocate()
						)->cpu_handle, 
						rtv_desc
					);
					break;

				case D3D12Resource::ViewType::DSV:
					dsv_desc.Format = desc.Format;
					dsv_desc.ViewDimension = dsv_dim;
					dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
  
					switch (dsv_dim) {
					case D3D12_DSV_DIMENSION_TEXTURE1D:
						dsv_desc.Texture1D.MipSlice = static_cast<UINT>(base_mip_level);
						break;
					case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
						dsv_desc.Texture1DArray.MipSlice = static_cast<UINT>(base_mip_level);
						dsv_desc.Texture1DArray.FirstArraySlice = static_cast<UINT>(base_layer);
						dsv_desc.Texture1DArray.ArraySize = static_cast<UINT>(layer_count);
						break;
					case D3D12_DSV_DIMENSION_TEXTURE2D:
						dsv_desc.Texture2D.MipSlice = static_cast<UINT>(base_mip_level);
						break;
					case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
						dsv_desc.Texture2DArray.MipSlice = static_cast<UINT>(base_mip_level);
						dsv_desc.Texture2DArray.FirstArraySlice = static_cast<UINT>(base_layer);
						dsv_desc.Texture2DArray.ArraySize = static_cast<UINT>(layer_count);
						break;
					default:
						throw std::invalid_argument("Unsupported DSV dimension");
					}
  

					logical_device.CreateDSV(
						raw_resource, 
						handle.emplace<D3D12DSVAllocator::Allocation>(
							logical_device.GetDepthStencilViewAllocator()->Allocate()
						)->cpu_handle, 
						dsv_desc
					);

					break;

				default:
					throw std::invalid_argument("Texture cannot be used for this view type");
				}
  
				return handle;
				
			}()) {
	}

	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> D3D12View::GetNative() const noexcept {
		return std::visit(
			[](auto&& handle) -> std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> {
				if constexpr (!std::same_as<std::monostate, std::decay_t<decltype(handle)>>) {
					return { handle->cpu_handle, handle->gpu_handle };
				}
				else {
					return {};
				}
			},
			m_handle
		);
	}

}

#endif // defined(_WIN32)