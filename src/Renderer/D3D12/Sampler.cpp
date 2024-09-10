
#include "pch.h"
#include "Sampler.h"
#include "Framework/Utility/Hash.h"
#include "DescriptorHeap.h"

using namespace Fyuu::core;
using namespace Fyuu::utility;
using namespace Fyuu::graphics::d3d12;

static std::map<std::size_t, D3D12_CPU_DESCRIPTOR_HANDLE> s_sampler_cache;

static std::unordered_map<SamplerType, Sampler> s_samplers = {

	{SamplerType::SAMPLER_LINEAR_WRAP,		Sampler{}},

	{SamplerType::SAMPLER_ANISO_WRAP,		Sampler{}},

	{SamplerType::SAMPLER_SHADOW,			Sampler{}},

	{SamplerType::SAMPLER_LINEAR_CLAMP,		Sampler{}},

	{SamplerType::SAMPLER_VOLUME_WRAP,		Sampler{}},

	{SamplerType::SAMPLER_POINT_CLAMP,		Sampler{}},

	{SamplerType::SAMPLER_LINEAR_BORDER,	Sampler{}},

	{SamplerType::SAMPLER_POINT_BORDER,		Sampler{}},


};

D3D12_CPU_DESCRIPTOR_HANDLE Fyuu::graphics::d3d12::SamplerDesc::CreateDescriptor() const {

	std::size_t hash = Hash(this);
	auto it = s_sampler_cache.find(hash);
	if (it != s_sampler_cache.end()) {
		return it->second;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE handle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	D3D12Device()->CreateSampler(this, handle);
	return handle;

}

void Fyuu::graphics::d3d12::SamplerDesc::CreateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle) {

	if (!handle.ptr || handle.ptr == -1) {
		throw std::invalid_argument("Invalid handle");
	}

	D3D12Device()->CreateSampler(this, handle);

}

void Fyuu::graphics::d3d12::InitializeD3D12Samplers() {

	s_samplers[SamplerType::SAMPLER_LINEAR_WRAP].desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	s_samplers[SamplerType::SAMPLER_LINEAR_WRAP].handle = s_samplers[SamplerType::SAMPLER_LINEAR_WRAP].desc.CreateDescriptor();

	s_samplers[SamplerType::SAMPLER_ANISO_WRAP].desc.MaxAnisotropy = 4;
	s_samplers[SamplerType::SAMPLER_ANISO_WRAP].handle = s_samplers[SamplerType::SAMPLER_ANISO_WRAP].desc.CreateDescriptor();

	s_samplers[SamplerType::SAMPLER_SHADOW].desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	s_samplers[SamplerType::SAMPLER_SHADOW].desc.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	s_samplers[SamplerType::SAMPLER_SHADOW].desc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	s_samplers[SamplerType::SAMPLER_SHADOW].handle = s_samplers[SamplerType::SAMPLER_SHADOW].desc.CreateDescriptor();

	s_samplers[SamplerType::SAMPLER_LINEAR_CLAMP].desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	s_samplers[SamplerType::SAMPLER_LINEAR_CLAMP].desc.SetBorderColor(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	s_samplers[SamplerType::SAMPLER_LINEAR_CLAMP].handle = s_samplers[SamplerType::SAMPLER_LINEAR_CLAMP].desc.CreateDescriptor();

	s_samplers[SamplerType::SAMPLER_VOLUME_WRAP].desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	s_samplers[SamplerType::SAMPLER_VOLUME_WRAP].handle = s_samplers[SamplerType::SAMPLER_VOLUME_WRAP].desc.CreateDescriptor();

	s_samplers[SamplerType::SAMPLER_POINT_CLAMP].desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	s_samplers[SamplerType::SAMPLER_POINT_CLAMP].desc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	s_samplers[SamplerType::SAMPLER_POINT_CLAMP].handle = s_samplers[SamplerType::SAMPLER_POINT_CLAMP].desc.CreateDescriptor();

	s_samplers[SamplerType::SAMPLER_LINEAR_BORDER].desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	s_samplers[SamplerType::SAMPLER_LINEAR_BORDER].desc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
	s_samplers[SamplerType::SAMPLER_LINEAR_BORDER].desc.SetBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
	s_samplers[SamplerType::SAMPLER_LINEAR_BORDER].handle = s_samplers[SamplerType::SAMPLER_LINEAR_BORDER].desc.CreateDescriptor();

	s_samplers[SamplerType::SAMPLER_POINT_BORDER].desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	s_samplers[SamplerType::SAMPLER_POINT_BORDER].desc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
	s_samplers[SamplerType::SAMPLER_POINT_BORDER].desc.SetBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
	s_samplers[SamplerType::SAMPLER_POINT_BORDER].handle = s_samplers[SamplerType::SAMPLER_POINT_BORDER].desc.CreateDescriptor();


}
