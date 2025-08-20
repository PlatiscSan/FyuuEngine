module;

#ifdef WIN32
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <comdef.h>
#include "d3dx12.h"
#endif // WIN32

#ifdef interface
#undef interface
#endif // interface

module graphics:d3d12_command_object;
import std;

namespace graphics::api::d3d12 {

	void ThrowIfFailed(HRESULT hr) {
		if (FAILED(hr)) {
#ifdef _DEBUG
#ifdef UNICODE
			auto str = std::wformat(L"**ERROR** Fatal Error with HRESULT of {}", hr);
#else
			auto str = std::format("**ERROR** Fatal Error with HRESULT of {}", hr);
#endif
			OutputDebugString(str.data());
			__debugbreak();
#endif
			throw _com_error(hr);
		}
	}

	static Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateDirectCommandAllocator(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device
	) {
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator)));
		return command_allocator;
	}

	static Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateDirectGraphicsCommandList(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> const& command_allocator
	) {
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list;
		ThrowIfFailed(
			device->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				command_allocator.Get(),
				nullptr,
				IID_PPV_ARGS(&command_list)
			)
		);
		ThrowIfFailed(command_list->Close());
		return command_list;
	}

	std::vector<D3D12CommandObject> D3D12CommandObject::CreateCommandObjects(
		std::size_t count,
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		util::MessageBusPtr const& message_bus
	) {
		std::vector<D3D12CommandObject> command_objects;
		command_objects.reserve(count);
		for (std::size_t i = 0; i < count; ++i) {
			command_objects.emplace_back(device, message_bus);
		}
		return command_objects;
	}

	D3D12CommandObject::D3D12CommandObject(Microsoft::WRL::ComPtr<ID3D12Device> const& device, util::MessageBusPtr const& message_bus)
		: m_command_allocator(CreateDirectCommandAllocator(device)),
		m_command_list(CreateDirectGraphicsCommandList(device, m_command_allocator)),
		m_message_bus(util::MakeReferred(message_bus)) {
	}

	void* D3D12CommandObject::GetNativeHandle() const noexcept {
		return m_command_list.Get();
	}

	API D3D12CommandObject::GetAPI() const noexcept {
		return API::DirectX12;
	}

	void D3D12CommandObject::StartRecording() {
		ThrowIfFailed(m_command_allocator->Reset());
		ThrowIfFailed(m_command_list->Reset(m_command_allocator.Get(), nullptr));
	}

	void D3D12CommandObject::EndRecording() {
		ThrowIfFailed(m_command_list->Close());
		// The command list is now ready to be executed.
		// It can be submitted to the command queue for execution.
		// This is typically done by the render device or a similar manager.
		if (m_message_bus) {
			CommandReadyEvent event{ m_command_list };
			m_message_bus->Publish(event);
		}
	}

	void D3D12CommandObject::Reset() {
		ThrowIfFailed(m_command_allocator->Reset());
		ThrowIfFailed(m_command_list->Reset(m_command_allocator.Get(), nullptr));
		m_command_list->Close(); // Resetting the command list also closes it.
	}

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> D3D12CommandObject::GetCommandList() const noexcept {
		return m_command_list;
	}

}