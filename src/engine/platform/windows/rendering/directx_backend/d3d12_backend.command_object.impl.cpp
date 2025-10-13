module;
#ifdef WIN32
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <comdef.h>
#include <directx/d3dx12.h>
#endif // WIN32

module d3d12_backend:command_object;
import :util;
import rendering;
import static_hash_map;

namespace fyuu_engine::windows::d3d12 {
#ifdef WIN32
	static Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateDirectCommandAllocator(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device
	) {
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator)));
		return command_allocator;
	}

	static Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> CreateDirectGraphicsCommandList(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> const& command_allocator
	) {
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> command_list;
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

	static D3D12_RESOURCE_STATES ResourceStateToD3D12ResourceState(core::ResourceState state) try {
		
		static concurrency::StaticHashMap<core::ResourceState, D3D12_RESOURCE_STATES, 100> map = {
			{core::ResourceState::Common, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON},
			{core::ResourceState::VertexBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER},
			{core::ResourceState::IndexBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER},
			{core::ResourceState::Present, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT},
			{core::ResourceState::OutputTarget, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET},
			{core::ResourceState::CopySrc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE},
			{core::ResourceState::CopyDest, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST},
		};

		return map.UnsafeAt(state);

	}
	catch (std::out_of_range const&) {
		return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
	}

	static D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopologyToD3D12PrimitiveTopology(core::PrimitiveTopology primitive_topology) try {

		static concurrency::StaticHashMap<core::PrimitiveTopology, D3D12_PRIMITIVE_TOPOLOGY, 100> map = {
			{core::PrimitiveTopology::Undefined, D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED},
			{core::PrimitiveTopology::PointList, D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_POINTLIST},
			{core::PrimitiveTopology::LineList, D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_LINELIST},
			{core::PrimitiveTopology::LineStrip, D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINESTRIP},
			{core::PrimitiveTopology::TriangleList, D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST},
			{core::PrimitiveTopology::TriangleStrip, D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP},
		};

		return map.UnsafeAt(primitive_topology);
	}
	catch (std::out_of_range const&) {
		return D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}

	void D3D12CommandObject::BeginRecordingImpl() {
		ThrowIfFailed(m_command_allocator->Reset());
		ThrowIfFailed(m_command_list->Reset(m_command_allocator.Get(), nullptr));
	}

	void D3D12CommandObject::BeginRecordingImpl(D3D12PipelineStateObject const& pso) {
		ThrowIfFailed(m_command_allocator->Reset());
		ThrowIfFailed(m_command_list->Reset(m_command_allocator.Get(), pso.pipeline_state.Get()));
	}

	void D3D12CommandObject::EndRecordingImpl() {
		ThrowIfFailed(m_command_list->Close());
		// The command list is now ready to be executed.
		// It can be submitted to the command queue for execution.
		// This is typically done by the render device or a similar manager.
		if (m_message_bus) {
			m_message_bus->Publish<D3D12CommandListReady>(static_cast<ID3D12CommandList*>(m_command_list.Get()));
		}
	}

	void D3D12CommandObject::ResetImpl() {
		ThrowIfFailed(m_command_allocator->Reset());
		ThrowIfFailed(m_command_list->Reset(m_command_allocator.Get(), nullptr));
		m_command_list->Close(); // Resetting the command list also closes it.
	}

	void D3D12CommandObject::SetViewportImpl(core::Viewport const& viewport) {

		auto d3d12_viewport = reinterpret_cast<D3D12_VIEWPORT const*>(&viewport);

		m_command_list->RSSetViewports(1u, d3d12_viewport);

	}

	void D3D12CommandObject::SetScissorRectImpl(core::Rect const& rect) {

		D3D12_RECT d3d12_rect;
		d3d12_rect.left = rect.x;
		d3d12_rect.top = rect.y;
		d3d12_rect.bottom = rect.height;
		d3d12_rect.right = rect.width;

		m_command_list->RSSetScissorRects(1u, &d3d12_rect);

	}

	void D3D12CommandObject::BarrierImpl(D3D12Buffer const& buffer, core::ResourceBarrierDesc const& desc) {
		
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			buffer.Native(),
			ResourceStateToD3D12ResourceState(desc.before),
			ResourceStateToD3D12ResourceState(desc.after)
		);

		m_command_list->ResourceBarrier(1u, &barrier);

	}

	void D3D12CommandObject::BarrierImpl(D3D12RenderTarget const& render_target, core::ResourceBarrierDesc const& desc) {

		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			render_target.buffer.Get(),
			ResourceStateToD3D12ResourceState(desc.before),
			ResourceStateToD3D12ResourceState(desc.after)
		);

		m_command_list->ResourceBarrier(1u, &barrier);

	}

	void D3D12CommandObject::BarrierImpl(std::monostate const& texture, core::ResourceBarrierDesc const& desc) {
		/*
		*	TODO: replace with real texture
		*/
	}

	void D3D12CommandObject::BeginRenderPassImpl(D3D12RenderTarget const& render_target, std::span<float> clear_value) {

		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			render_target.buffer.Get(),
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET
		);

		m_command_list->ResourceBarrier(1u, &barrier);

		CD3DX12_CLEAR_VALUE d3d12_clear_value(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, clear_value.data());

		D3D12_RENDER_PASS_BEGINNING_ACCESS render_pass_beginning_access_clear{ D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR, { d3d12_clear_value } };
		D3D12_RENDER_PASS_ENDING_ACCESS render_pass_ending_access_preserve{ D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE, {} };
		D3D12_RENDER_PASS_RENDER_TARGET_DESC render_pass_render_target_desc{ render_target.render_target_descriptor, render_pass_beginning_access_clear, render_pass_ending_access_preserve };
		
		m_command_list->BeginRenderPass(1, &render_pass_render_target_desc, nullptr, D3D12_RENDER_PASS_FLAG_NONE);

	}

	void D3D12CommandObject::EndRenderPassImpl(D3D12RenderTarget const& render_target) {

		m_command_list->EndRenderPass();

		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			render_target.buffer.Get(),
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT
		);

		m_command_list->ResourceBarrier(1u, &barrier);

	}

	void D3D12CommandObject::BindVertexBufferImpl(D3D12Buffer const& buffer, core::VertexDesc const& desc) {
		D3D12_VERTEX_BUFFER_VIEW view;
		view.BufferLocation = buffer.GPUAddress();
		view.SizeInBytes = desc.size;
		view.StrideInBytes = desc.stride;
		m_command_list->IASetVertexBuffers(desc.slot, 1, &view);
	}

	void D3D12CommandObject::SetPrimitiveTopologyImpl(core::PrimitiveTopology primitive_topology) {
		m_command_list->IASetPrimitiveTopology(PrimitiveTopologyToD3D12PrimitiveTopology(primitive_topology));
	}

	void D3D12CommandObject::DrawImpl(
		std::uint32_t index_count, std::uint32_t instance_count, 
		std::uint32_t start_index, std::int32_t base_vertex,
		std::uint32_t start_instance
	) {

		m_command_list->DrawInstanced(index_count, instance_count, base_vertex + start_index, start_instance);

	}

	void D3D12CommandObject::ClearImpl(D3D12RenderTarget const& render_target, std::span<float> rgba, core::Rect const& rect) {

		D3D12_RECT d3d12_rect;
		d3d12_rect.left = rect.x;
		d3d12_rect.top = rect.y;
		d3d12_rect.bottom = rect.height;
		d3d12_rect.right = rect.width;
		
		m_command_list->ClearRenderTargetView(render_target.render_target_descriptor, rgba.data(), 1u, &d3d12_rect);
	}

	void D3D12CommandObject::CopyImpl(D3D12Buffer const& src, D3D12Buffer const& dest) {
		m_command_list->CopyBufferRegion(dest.Native(), 0, src.Native(), 0, src.Size());
	}

	D3D12CommandObject::D3D12CommandObject(
		Microsoft::WRL::ComPtr<ID3D12Device> const& device,
		util::PointerWrapper<concurrency::SyncMessageBus> const& message_bus
	) : m_command_allocator(CreateDirectCommandAllocator(device)),
		m_command_list(CreateDirectGraphicsCommandList(device, m_command_allocator)),
		m_message_bus(util::MakeReferred(message_bus)) {
	}

	void D3D12CommandObject::SetGraphicsRootSignature(ID3D12RootSignature* root_signature) {
		m_command_list->SetGraphicsRootSignature(root_signature);
	}

	void D3D12CommandObject::SetGraphicsRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> const& root_signature) {
		m_command_list->SetGraphicsRootSignature(root_signature.Get());
	}

#endif // WIN32

}