module;
#ifdef WIN32
#include <dxgi1_6.h>
#include <directx/d3dx12.h>
#include <wrl.h>
#include <comdef.h>
#endif // WIN32

module fyuu_rhi:d3d12_pipeline_layout;
import :d3d12_logical_device;

static constexpr std::size_t RootParameterBufferSize = sizeof(CD3DX12_ROOT_PARAMETER1) * 128u;
static constexpr std::size_t DescriptorRangeBufferSize = sizeof(D3D12_DESCRIPTOR_RANGE1) * 128u;

namespace fyuu_rhi::d3d12 {

	static Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(
		D3D12LogicalDevice const& logical_device,
		PipelineLayoutDescriptor const& descriptor
	) {

		/*
		*	pool for root parameter allocation and descriptor ranges
		*/

		std::array<std::byte, RootParameterBufferSize> root_param_buffer;
		std::pmr::monotonic_buffer_resource root_param_buffer_pool(&root_param_buffer, root_param_buffer.size());
		std::pmr::polymorphic_allocator<CD3DX12_ROOT_PARAMETER1> root_param_alloc(&root_param_buffer_pool);

		std::array<std::byte, DescriptorRangeBufferSize> desc_range_buffer;
		std::pmr::monotonic_buffer_resource desc_range_buffer_pool(&desc_range_buffer, desc_range_buffer.size());
		std::pmr::polymorphic_allocator<D3D12_DESCRIPTOR_RANGE1> desc_range_alloc(&desc_range_buffer_pool);


		// Create a root signature.
		// version 1.1 is preferred, but we fall back to 1.0 if it is not available

		D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};

		HRESULT result = logical_device.GetRawNative()->CheckFeatureSupport(
			D3D12_FEATURE_ROOT_SIGNATURE,
			&feature_data, sizeof(feature_data)
		);

		feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(result)) {
			feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}


		// Allow input layout and deny unnecessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// create the constants data

		std::pmr::vector<CD3DX12_ROOT_PARAMETER1> root_parameters(root_param_alloc);
		std::pmr::vector<D3D12_DESCRIPTOR_RANGE1> ranges(desc_range_alloc); // stable memory location is necessary

		for (auto const& constant : descriptor.constants) {

			D3D12_ROOT_PARAMETER1 constant_root_param;

			CD3DX12_ROOT_PARAMETER1::InitAsConstantBufferView(
				constant_root_param,
				constant.register_num,
				0,
				D3D12_ROOT_DESCRIPTOR_FLAGS::D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
				D3D12_SHADER_VISIBILITY_ALL
			);

			root_parameters.emplace_back(std::move(constant_root_param));

		}

		for (auto const& item : descriptor.bindings) {
			switch (item.type) {
			case BindingType::Sampler:
			{
				D3D12_DESCRIPTOR_RANGE1& range = ranges.emplace_back(
					D3D12_DESCRIPTOR_RANGE1{
						D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
						item.count,
						item.binding,
						0,
						D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
						D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
					}
				);
				root_parameters.emplace_back().InitAsDescriptorTable(1, &range);
			}
			break;

			case BindingType::SampledImage:
			{
				D3D12_DESCRIPTOR_RANGE1& range = ranges.emplace_back(
					D3D12_DESCRIPTOR_RANGE1{
						D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
						item.count,
						item.binding,
						0u,
						D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
						D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
					}
				);
				root_parameters.emplace_back().InitAsDescriptorTable(1, &range);
			}
			break;

			case BindingType::StorageImage:
			{
				D3D12_DESCRIPTOR_RANGE1& range = ranges.emplace_back(
					D3D12_DESCRIPTOR_RANGE1{
						D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
						item.count,
						item.is_bindless ? 0u : item.binding,
						item.is_bindless ? item.binding : 0u,
						D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
						D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
					}
				);
				root_parameters.emplace_back().InitAsDescriptorTable(1, &range);
			}

			case BindingType::UniformBuffer:
			case BindingType::StorageBuffer:
			{
				if (item.is_bindless) {
					if (item.writable) {
						auto& range = ranges.emplace_back(
							D3D12_DESCRIPTOR_RANGE1{
								D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
								item.count,
								0,
								item.binding,
								D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
								D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
							}
						);
						root_parameters.emplace_back().InitAsDescriptorTable(1, &range);
					}
					else {
						auto& range = ranges.emplace_back(
							D3D12_DESCRIPTOR_RANGE1{
								D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
								item.count,
								0,
								item.binding,
								D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
								D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
							}
						);
						root_parameters.emplace_back().InitAsDescriptorTable(1, &range);
					}
				}
			}

			default:
				break;
			}
		}


		// constant / uniform buffer bindings (SRVs)
		for (const auto& item : descriptor.bindings) {
			switch (item.type) {
			case BindingType::StorageBuffer:
			case BindingType::UniformBuffer:
				if (!item.is_bindless) {
					if (item.writable) {
						root_parameters.emplace_back().InitAsUnorderedAccessView(item.binding, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);
					}
					else {
						root_parameters.emplace_back().InitAsShaderResourceView(item.binding, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);
					}
				}
				break;

			}
		}

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_description;
		root_signature_description.Init_1_1(
			static_cast<UINT>(root_parameters.size()), root_parameters.data(),
			0, nullptr, 
			root_signature_flags
		);


		// Serialize the root signature.
		// it becomes a binary object which can be used to create the actual root signature
		Microsoft::WRL::ComPtr<ID3DBlob> root_signature_blob;
		Microsoft::WRL::ComPtr<ID3DBlob> error_blob;
		try {
			ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&root_signature_description, feature_data.HighestVersion, &root_signature_blob, &error_blob));
		}
		catch (_com_error const&) {
			throw std::runtime_error(std::format("{}", (char*)error_blob->GetBufferPointer()));
		}

		// Create the root signature.

		Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;

		ThrowIfFailed(
			logical_device.GetRawNative()->CreateRootSignature(
				0, 
				root_signature_blob->GetBufferPointer(),
				root_signature_blob->GetBufferSize(), 
				IID_PPV_ARGS(&root_signature)
			)
		);

		return root_signature;

	}
	
	D3D12PipelineLayout::D3D12PipelineLayout(
		plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device,
		PipelineLayoutDescriptor const& descriptor
	) : m_root_signature(CreateRootSignature(*logical_device, descriptor)) {
	}

	D3D12PipelineLayout::D3D12PipelineLayout(
		D3D12LogicalDevice const& logical_device,
		PipelineLayoutDescriptor const& descriptor
	) : m_root_signature(CreateRootSignature(logical_device, descriptor)) {
	}

}

namespace std {
	std::size_t hash<fyuu_rhi::d3d12::details::BindingKey>::operator()(fyuu_rhi::d3d12::details::BindingKey const& x) const {
		return std::size_t(x.binding) << 32u + x.space;
	}
}