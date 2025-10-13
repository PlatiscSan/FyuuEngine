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

export module d3d12_backend:command_object;
export import pointer_wrapper;
export import :buffer;
export import :pso;
import rendering;
import std;

namespace fyuu_engine::windows::d3d12 {
#ifdef WIN32

	export struct D3D12CommandListReady {
		ID3D12CommandList* list;
	};

	export struct D3D12RenderTarget {
		Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
		D3D12_CPU_DESCRIPTOR_HANDLE render_target_descriptor;
	};

	struct D3D12CommandObjectTraits {

		using PipelineStateObjectType = D3D12PipelineStateObject;
		using OutputTargetType = D3D12RenderTarget;
		using BufferType = D3D12Buffer;
		using TextureType = std::monostate;

	};

	export class D3D12CommandObject final 
		: public core::BaseCommandObject<D3D12CommandObject, D3D12CommandObjectTraits> {
		friend class core::BaseCommandObject<D3D12CommandObject, D3D12CommandObjectTraits>;
	private:
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_command_allocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_command_list;
		util::PointerWrapper<concurrency::SyncMessageBus> m_message_bus;

		void BeginRecordingImpl();
		void BeginRecordingImpl(D3D12PipelineStateObject const& pso);
		void EndRecordingImpl();
		void ResetImpl();
		void SetViewportImpl(core::Viewport const& viewport);
		void SetScissorRectImpl(core::Rect const& rect);
		void BarrierImpl(D3D12Buffer const& buffer, core::ResourceBarrierDesc const& desc);
		void BarrierImpl(D3D12RenderTarget const& render_target, core::ResourceBarrierDesc const& desc);
		void BarrierImpl(std::monostate const& texture, core::ResourceBarrierDesc const& desc);
		void BeginRenderPassImpl(D3D12RenderTarget const& render_target, std::span<float> clear_value);
		void EndRenderPassImpl(D3D12RenderTarget const& render_target);
		void BindVertexBufferImpl(D3D12Buffer const& buffer, core::VertexDesc const& desc);
		void SetPrimitiveTopologyImpl(core::PrimitiveTopology primitive_topology);
		void DrawImpl(
			std::uint32_t index_count, std::uint32_t instance_count,
			std::uint32_t start_index, std::int32_t base_vertex,
			std::uint32_t start_instance
		);
		void ClearImpl(D3D12RenderTarget const& render_target, std::span<float> rgba, core::Rect const& rect);
		void CopyImpl(D3D12Buffer const& src, D3D12Buffer const& dest);

	public:
		D3D12CommandObject(
			Microsoft::WRL::ComPtr<ID3D12Device> const& device, 
			util::PointerWrapper<concurrency::SyncMessageBus> const& message_bus
		);

		void SetGraphicsRootSignature(ID3D12RootSignature* root_signature);
		void SetGraphicsRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> const& root_signature);

	};

#endif // WIN32

}