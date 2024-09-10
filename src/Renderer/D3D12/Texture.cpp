
#include "pch.h"
#include "Texture.h"

using namespace Fyuu::utility;

Fyuu::graphics::d3d12::Texture2D::Texture2D(std::string const& name, std::uint32_t width, std::uint32_t height, std::size_t row_pitch_bytes, DXGI_FORMAT format, void const* data)
	:m_name(name), m_width(width), m_height(height){

	m_usage_state = D3D12_RESOURCE_STATE_COPY_DEST;

	D3D12_RESOURCE_DESC1 texture_desc = {};

	texture_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texture_desc.Width = m_width;
	texture_desc.Height = height;
	texture_desc.MipLevels = 1;
	texture_desc.Format = format;
	texture_desc.SampleDesc.Count = 1;
	texture_desc.SampleDesc.Quality = 0;
	texture_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texture_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heap_properties = {};
	heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heap_properties.CreationNodeMask = 1;
	heap_properties.VisibleNodeMask = 1;

	ThrowIfFailed(D3D12Device()->CreateCommittedResource2(&heap_properties, D3D12_HEAP_FLAG_NONE, &texture_desc, m_usage_state, nullptr, nullptr, IID_PPV_ARGS(&m_resource)));

	D3D12_SUBRESOURCE_DATA texture_resource = {};
	texture_resource.pData = data;
	texture_resource.RowPitch = row_pitch_bytes;
	texture_resource.SlicePitch = row_pitch_bytes * m_height;



}

