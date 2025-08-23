module;

#ifdef WIN32
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <comdef.h>
#endif // WIN32

#ifdef interface
#undef interface
#endif // interface

export module graphics:d3d12_command_object;
export import :interface;
export import message_bus;
export import circular_buffer;
import std;


namespace graphics::api::d3d12 {
#ifdef WIN32

	export extern void ThrowIfFailed(HRESULT hr);

	export struct CommandReadyEvent {
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list;
	};

	export class D3D12CommandObject final : public ICommandObject {
	private:
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_command_allocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_command_list;
		util::MessageBusPtr m_message_bus;

	public:
		static std::vector<D3D12CommandObject> CreateCommandObjects(
			std::size_t count,
			Microsoft::WRL::ComPtr<ID3D12Device> const& device,
			util::MessageBusPtr const& message_bus
		);

		D3D12CommandObject(Microsoft::WRL::ComPtr<ID3D12Device> const& device, util::MessageBusPtr const& message_bus);

		~D3D12CommandObject() noexcept override = default;
		void* GetNativeHandle() const noexcept override;
		API GetAPI() const noexcept override;
		void StartRecording() override;
		void EndRecording() override;
		void Reset() override;

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() const noexcept;

	};
#endif // WIN32
}