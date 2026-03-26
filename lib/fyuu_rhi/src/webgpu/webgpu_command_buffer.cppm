/* webgpu_command_buffer.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <vector>
#include <string_view>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "webgpu.hpp"
export module fyuu_rhi:webgpu_command_buffer;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :webgpu_common;
import :webgpu_future;
import :synchronization;

namespace fyuu_rhi::webgpu {

	export class WebGPUCommandBuffer
		: public PolymorphicCommandBufferBase,
		public WebGPUCommon<WebGPUCommandBuffer> {
	private:
		std::vector<UniqueCommand> m_commands;
		SynchronizationObject m_sync;
		bool m_recording_flag;

	public:
		WebGPUCommandBuffer(
			WebGPULogicalDevice const& logical_device,
			CommandObjectType queue_type,
			std::string_view debug_name = {}
		);

		WebGPUCommandBuffer& BeginRecording();
		void EndRecording();
		void Reset();

		WebGPUCommandBuffer& SetFrame(
			WebGPUResource const& frame, 
			WebGPUView const& frame_view
		);

		WebGPUCommandBuffer& BeginRendering(
			std::span<WebGPUResource const* const> rts, 
			std::span<WebGPUView const* const> rtvs, 
			std::span<LoadOp const> rt_loads, 
			std::span<StoreOp const> rt_stores, 
			std::span<std::array<float, 4u> const> rt_clears
		);

		WebGPUCommandBuffer& BeginRendering(
			std::span<WebGPUResource const* const> rts, 
			std::span<WebGPUView const* const> rtvs, 
			std::span<LoadOp const> rt_loads,
			std::span<StoreOp const> rt_stores,
			std::span<std::array<float, 4u> const> rt_clears,
			WebGPUResource const& ds, 
			WebGPUView const& dsv, 
			LoadOp depth_load,
			StoreOp depth_store,
			float depth_clear,
			LoadOp stencil_load,
			StoreOp stencil_store,
			std::uint8_t stencil_clear
		);


		WebGPUCommandBuffer& EndRendering();

		WebGPUCommandBuffer& SetViewports(std::span<CmdSetViewports::Viewport const> viewports);

		WebGPUCommandBuffer& SetScissors(std::span<CmdSetScissors::Scissor const> scissors);

		WebGPUCommandBuffer& Execute(WebGPUGPUExecutable const& gpu_exec);

		WebGPUCommandBuffer& DispatchComputingTask(
			std::size_t threads_x,
			std::size_t threads_y,
			std::size_t threads_z,
			std::size_t threads_per_thread_group_x = 1u,
			std::size_t threads_per_thread_group_y = 1u,
			std::size_t threads_per_thread_group_z = 1u
		);

		
		WebGPUCommandBuffer& BindVertexBuffers(std::span<WebGPUResource const* const> vertex_buffers, std::span<std::size_t const> wheres, std::span<std::size_t const> offsets, std::span<std::size_t const> strides);

		WebGPUCommandBuffer& DrawInstanced(
			std::size_t vertices,
			std::size_t instances = 1u,
			std::size_t start_vertex = 0u,
			std::size_t first_instance = 0u
		);

		WebGPUCommandBuffer& DrawIndexed(
			std::size_t indices,
			std::size_t instances = 1u,
			std::size_t first_index = 0u,
			std::size_t start_vertex = 0u,
			std::size_t first_instance = 0u
		);

		WebGPUCommandBuffer& BindBuffer(
			ShaderStage stages,
			std::size_t where,
			WebGPUResource const& buffer,
			std::size_t offset = 0u,
			std::size_t ns = 0
		);

		WebGPUCommandBuffer& BindBuffer(
			ShaderStage stages,
			std::size_t where,
			WebGPUResource const& buffer,
			WebGPUView const& texel_view,
			std::size_t offset = 0u,
			std::size_t ns = 0
		);

		WebGPUCommandBuffer& PushData(
			ShaderStage stages,
			std::size_t where,
			std::span<std::byte const> bytes,
			std::size_t offset = 0u,
			std::size_t ns = 0u
		);

		WebGPUCommandBuffer& BindSampler(
			ShaderStage stages,
			std::size_t where,
			WebGPUSampler const& sampler,
			std::size_t ns = 0u
		);

		WebGPUCommandBuffer& BindTexture(
			ShaderStage stages,
			std::size_t where,
			WebGPUResource const& texture,
			WebGPUView const& view,
			std::size_t ns = 0u
		);

		WebGPUCommandBuffer& CopyTextureToBuffer(
			WebGPUResource const& src_texture,
			WebGPUResource const& dest_buffer,
			CmdCopyTextureToBuffer::Subresource const& src_subresource,
			CmdCopyTextureToBuffer::Offset3D const& src_offset,
			std::size_t dest_offset,
			CmdCopyTextureToBuffer::Extent3D const& extent
		);
		
		WebGPUCommandBuffer& CopyBufferToTexture(
			WebGPUResource const& src_buffer,
			WebGPUResource const& dest_texture,
			std::size_t src_offset,
			CmdCopyBufferToTexture::Subresource const& dest_subresource,
			CmdCopyBufferToTexture::Offset3D const& dest_offset,
			CmdCopyBufferToTexture::Extent3D const& extent
		);
		
		WebGPUCommandBuffer& CopyBufferToBuffer(
			WebGPUResource const& src_buffer,
			WebGPUResource const& dest_buffer,
			std::size_t src_offset,
			std::size_t dest_offset,
			std::size_t size
		);
		
		WebGPUCommandBuffer& CopyTextureToTexture(
			WebGPUResource const& src_texture,
			WebGPUResource const& dest_texture,
			CmdCopyTextureToTexture::Subresource const& src_subresource,
			CmdCopyTextureToTexture::Subresource const& dest_subresource,
			CmdCopyTextureToTexture::Offset3D const& src_offset,
			CmdCopyTextureToTexture::Offset3D const& dest_offset,
			CmdCopyTextureToTexture::Extent3D const& extent
		);

		WebGPUCommandBuffer& BindIndexBuffer(
			WebGPUResource const& index_buffer
		);
		
		WebGPUCommandBuffer& ExecuteIndirect(
			WebGPUResource const& argument_buffer,
			std::size_t offset_into_buffer,
			std::size_t draw_count
		);
		
		WebGPUCommandBuffer& ExecuteIndirectIndexed(
			WebGPUResource const& argument_buffer,
			std::size_t offset_into_buffer,
			std::size_t draw_count
		);
		
		WebGPUCommandBuffer& DispatchIndirect(
			WebGPUResource const& argument_buffer,
			std::size_t offset_into_buffer,
			std::size_t block_size_x,
			std::size_t block_size_y,
			std::size_t block_size_z
		);
		

	};

} // namespace fyuu_rhi::webgpu