#ifndef COMMAND_CONTEXT_H
#define COMMAND_CONTEXT_H

#include "CommandListManager.h"
#include "GPUResource.h"

namespace Fyuu::graphics::d3d12 {

	class CommandContext {

	public:

		CommandContext(D3D12_COMMAND_LIST_TYPE type);

		void Reset();

		D3D12_COMMAND_LIST_TYPE const& GetCommandListType() const noexcept { return m_type; }

	protected:

		void BindDescriptorHeaps();

		CommandObject m_cmd_object;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_cur_graphics_root_signature;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_cur_compute_root_signature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_cur_pipeline_state;

		D3D12_COMMAND_LIST_TYPE m_type;	
		std::uint32_t m_num_barriers_to_flush;

		std::array<D3D12_RESOURCE_BARRIER, 16> m_resource_barrier_buffer;
		std::array<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES>  m_current_descriptor_heaps;


	};

	std::shared_ptr<CommandContext> AllocateCommandContext(D3D12_COMMAND_LIST_TYPE type);
	void FreeCommandContext(std::shared_ptr<CommandContext>& context);
	void InitializeTexture(GPUResource& dest, std::uint32_t num_sub_resources, D3D12_SUBRESOURCE_DATA* sub_data);

}

#endif // !COMMAND_CONTEXT_H
