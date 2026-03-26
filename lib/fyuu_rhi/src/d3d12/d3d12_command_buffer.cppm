/* d3d12_command_buffer.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <vector>
#include <string_view>
#include <optional>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
export module fyuu_rhi:d3d12_command_buffer;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :synchronization;
import :types;
import :d3d12_future;
import :d3d12_common;

namespace fyuu_rhi::d3d12 {
	export class D3D12CommandBuffer
		: public PolymorphicCommandBufferBase,
		public D3D12Common<D3D12CommandBuffer> {
	private:
		std::optional<std::size_t> m_logical_device_id;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_impl;
		std::vector<UniqueCommand> m_commands;
		SynchronizationObject m_sync;
		bool m_recording_flag;

	public:
		D3D12CommandBuffer(
			D3D12LogicalDevice const& logical_device,
			CommandObjectType type,
			std::wstring_view debug_name = {}
		);

		D3D12CommandBuffer& BeginRecording();
		void EndRecording();
		void Reset();

		D3D12CommandBuffer& SetFrame(
			D3D12Resource const& frame, 
			D3D12View const& frame_view
		);

		D3D12CommandBuffer& BeginRendering(
			std::span<D3D12Resource const* const> rts, 
			std::span<D3D12View const* const> rtvs, 
			std::span<LoadOp const> rt_loads, 
			std::span<StoreOp const> rt_stores, 
			std::span<std::array<float, 4u> const> rt_clears
		);

		D3D12CommandBuffer& BeginRendering(
			std::span<D3D12Resource const* const> rts, 
			std::span<D3D12View const* const> rtvs, 
			std::span<LoadOp const> rt_loads,
			std::span<StoreOp const> rt_stores,
			std::span<std::array<float, 4u> const> rt_clears,
			D3D12Resource const& ds, 
			D3D12View const& dsv, 
			LoadOp depth_load,
			StoreOp depth_store,
			float depth_clear,
			LoadOp stencil_load,
			StoreOp stencil_store,
			std::uint8_t stencil_clear
		);

		D3D12CommandBuffer& EndRendering();

		D3D12CommandBuffer& SetViewports(std::span<CmdSetViewports::Viewport const> viewports);

		D3D12CommandBuffer& SetScissors(std::span<CmdSetScissors::Scissor const> scissors);

		D3D12CommandBuffer& Execute(D3D12GPUExecutable const& gpu_exec);

		D3D12CommandBuffer& DispatchComputingTask(
			std::size_t threads_x,
			std::size_t threads_y,
			std::size_t threads_z,
			std::size_t threads_per_thread_group_x = 1u,
			std::size_t threads_per_thread_group_y = 1u,
			std::size_t threads_per_thread_group_z = 1u
		);
		
		D3D12CommandBuffer& BindVertexBuffers(std::span<D3D12Resource const* const> vertex_buffers, std::span<std::size_t const> wheres, std::span<std::size_t const> offsets, std::span<std::size_t const> strides);

		D3D12CommandBuffer& DrawInstanced(
			std::size_t vertices,
			std::size_t instances = 1u,
			std::size_t start_vertex = 0u,
			std::size_t first_instance = 0u
		);

		D3D12CommandBuffer& DrawIndexed(
			std::size_t indices,
			std::size_t instances = 1u,
			std::size_t first_index = 0u,
			std::size_t start_vertex = 0u,
			std::size_t first_instance = 0u
		);

		D3D12CommandBuffer& BindBuffer(
			ShaderStage stages,
			std::size_t where,
			D3D12Resource const& buffer,
			std::size_t offset = 0u,
			std::size_t ns = 0
		);

		D3D12CommandBuffer& BindBuffer(
			ShaderStage stages,
			std::size_t where,
			D3D12Resource const& buffer,
			D3D12View const& texel_view,
			std::size_t offset = 0u,
			std::size_t ns = 0
		);

		D3D12CommandBuffer& PushData(
			ShaderStage stages,
			std::size_t where,
			std::span<std::byte const> bytes,
			std::size_t offset = 0u, 
			std::size_t ns = 0
		);

		D3D12CommandBuffer& BindSampler(
			ShaderStage stages,
			std::size_t where,
			D3D12Sampler const& sampler,
			std::size_t ns = 0u
		);

		D3D12CommandBuffer& BindTexture(
			ShaderStage stages,
			std::size_t where,
			D3D12Resource const& texture,
			D3D12View const& view,
			std::size_t ns = 0u
		);

		D3D12CommandBuffer& CopyTextureToBuffer(
			D3D12Resource const& src_texture,
			D3D12Resource const& dest_buffer,
			CmdCopyTextureToBuffer::Subresource const& src_subresource,
			CmdCopyTextureToBuffer::Offset3D const& src_offset,
			std::size_t dest_offset,
			CmdCopyTextureToBuffer::Extent3D const& extent
		);
		
		D3D12CommandBuffer& CopyBufferToTexture(
			D3D12Resource const& src_buffer,
			D3D12Resource const& dest_texture,
			std::size_t src_offset,
			CmdCopyBufferToTexture::Subresource const& dest_subresource,
			CmdCopyBufferToTexture::Offset3D const& dest_offset,
			CmdCopyBufferToTexture::Extent3D const& extent
		);
		
		D3D12CommandBuffer& CopyBufferToBuffer(
			D3D12Resource const& src_buffer,
			D3D12Resource const& dest_buffer,
			std::size_t src_offset,
			std::size_t dest_offset,
			std::size_t size
		);
		
		D3D12CommandBuffer& CopyTextureToTexture(
			D3D12Resource const& src_texture,
			D3D12Resource const& dest_texture,
			CmdCopyTextureToTexture::Subresource const& src_subresource,
			CmdCopyTextureToTexture::Subresource const& dest_subresource,
			CmdCopyTextureToTexture::Offset3D const& src_offset,
			CmdCopyTextureToTexture::Offset3D const& dest_offset,
			CmdCopyTextureToTexture::Extent3D const& extent
		);

		D3D12CommandBuffer& BindIndexBuffer(
			D3D12Resource const& index_buffer
		);
		
		D3D12CommandBuffer& ExecuteIndirect(
			D3D12Resource const& argument_buffer,
			std::size_t offset_into_buffer,
			std::size_t draw_count
		);
		
		D3D12CommandBuffer& ExecuteIndirectIndexed(
			D3D12Resource const& argument_buffer,
			std::size_t offset_into_buffer,
			std::size_t draw_count
		);
		
		D3D12CommandBuffer& DispatchIndirect(
			D3D12Resource const& argument_buffer,
			std::size_t offset_into_buffer,
			std::size_t block_size_x,
			std::size_t block_size_y,
			std::size_t block_size_z
		);
	

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> GetNative() const noexcept;
		
		ID3D12GraphicsCommandList4* GetRawNative() const noexcept;


	};
}

#endif // defined(_WIN32)