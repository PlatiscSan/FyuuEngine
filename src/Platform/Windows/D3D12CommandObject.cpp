
#include "pch.h"
#include "D3D12CommandObject.h"

using namespace Fyuu::windows_util;

Fyuu::D3D12CommandObject::D3D12CommandObject(D3D12Device* device, D3D12_COMMAND_LIST_TYPE type) {

	D3D12_COMMAND_QUEUE_DESC desc = {
		type,
		D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
		D3D12_COMMAND_QUEUE_FLAG_NONE,
		0
	};
	ThrowIfFailed(device->GetID3D12Device()->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_command_queue)));

	for (auto& allocator : m_allocators) {
		ThrowIfFailed(device->GetID3D12Device()->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator)));
	}

	ThrowIfFailed(device->GetID3D12Device()->CreateCommandList(0, type, m_allocators[0].Get(), nullptr, IID_PPV_ARGS(&m_command_list)));
	ThrowIfFailed(m_command_list->Close());

}
