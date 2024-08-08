#ifndef D3D12_COMMAND_OBJECT_H
#define D3D12_COMMAND_OBJECT_H

#include "Framework/Renderer/CommandObject.h"
#include "D3D12Device.h"

namespace Fyuu {

	class D3D12CommandObject final : public CommandObject {

	public:

		D3D12CommandObject(D3D12Device* device, D3D12_COMMAND_LIST_TYPE type);

		static std::size_t const num_frame_buffers = 3;

	private:

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_command_queue;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> m_command_list;
		std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, num_frame_buffers> m_allocators;

	};

}

#endif // !D3D12_COMMAND_OBJECT_H
