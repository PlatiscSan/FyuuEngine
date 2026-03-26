/* opengl_command_queue.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <shared_mutex>
#include <array>
#include <vector>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include "glad.hpp"
export module fyuu_rhi:opengl_command_buffer;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :opengl_common;
import :synchronization;

namespace fyuu_rhi::opengl {
	
	export class OpenGLCommandBuffer 
		: public PolymorphicCommandBufferBase,
		public OpenGLCommon<OpenGLCommandBuffer> {
	private:
		std::vector<UniqueCommand> m_commands;
		SynchronizationObject m_sync;
		bool m_recording_flag;
		
	public:
		OpenGLCommandBuffer(
			OpenGLLogicalDevice const& logical_device, 			
			CommandObjectType queue_type = CommandObjectType::AllCommands
		);

		std::span<UniqueCommand const> GetCommands() const noexcept;
		OpenGLCommandBuffer& BeginRecording();
		void EndRecording();
		void Reset();

		OpenGLCommandBuffer& SetFrame(
			OpenGLResource const& frame, 
			OpenGLView const& frame_view
		);

		OpenGLCommandBuffer& BeginRendering(
			std::span<OpenGLResource const* const> rts, 
			std::span<OpenGLView const* const> rtvs, 
			std::span<LoadOp const> rt_loads, 
			std::span<StoreOp const> rt_stores, 
			std::span<std::array<float, 4u> const> rt_clears
		);

		OpenGLCommandBuffer& BeginRendering(
			std::span<OpenGLResource const* const> rts, 
			std::span<OpenGLView const* const> rtvs, 
			std::span<LoadOp const> rt_loads,
			std::span<StoreOp const> rt_stores,
			std::span<std::array<float, 4u> const> rt_clears,
			OpenGLResource const& ds, 
			OpenGLView const& dsv, 
			LoadOp depth_load,
			StoreOp depth_store,
			float depth_clear,
			LoadOp stencil_load,
			StoreOp stencil_store,
			std::uint8_t stencil_clear
		);

		OpenGLCommandBuffer& EndRendering();

		OpenGLCommandBuffer& SetViewports(std::span<CmdSetViewports::Viewport const> viewports);

		OpenGLCommandBuffer& SetScissors(std::span<CmdSetScissors::Scissor const> scissors);

		OpenGLCommandBuffer& Execute(OpenGLGPUExecutable const& gpu_exec);

		OpenGLCommandBuffer& DispatchComputingTask(
			std::size_t threads_x,
			std::size_t threads_y,
			std::size_t threads_z,
			std::size_t threads_per_thread_group_x = 1u,
			std::size_t threads_per_thread_group_y = 1u,
			std::size_t threads_per_thread_group_z = 1u
		);

		
		OpenGLCommandBuffer& BindVertexBuffers(std::span<OpenGLResource const* const> vertex_buffers, std::span<std::size_t const> wheres, std::span<std::size_t const> offsets, std::span<std::size_t const> strides);

		OpenGLCommandBuffer& DrawInstanced(
			std::size_t vertices,
			std::size_t instances = 1u,
			std::size_t start_vertex = 0u,
			std::size_t first_instance = 0u
		);

		OpenGLCommandBuffer& DrawIndexed(
			std::size_t indices,
			std::size_t instances = 1u,
			std::size_t first_index = 0u,
			std::size_t start_vertex = 0u,
			std::size_t first_instance = 0u
		);

		OpenGLCommandBuffer& BindBuffer(
			ShaderStage stages,
			std::size_t where,
			OpenGLResource const& buffer,
			std::size_t offset = 0u,
			std::size_t ns = 0
		);

		OpenGLCommandBuffer& BindBuffer(
			ShaderStage stages,
			std::size_t where,
			OpenGLResource const& buffer,
			OpenGLView const& texel_view,
			std::size_t offset = 0u,
			std::size_t ns = 0
		);

		OpenGLCommandBuffer& PushData(
			ShaderStage stages,
			std::size_t where,
			std::span<std::byte const> bytes,
			std::size_t offset = 0u,
			std::size_t ns = 0u
		);

		OpenGLCommandBuffer& BindSampler(
			ShaderStage stages,
			std::size_t where,
			OpenGLSampler const& sampler,
			std::size_t ns = 0u
		);

		OpenGLCommandBuffer& BindTexture(
			ShaderStage stages,
			std::size_t where,
			OpenGLResource const& texture,
			OpenGLView const& view,
			std::size_t ns = 0u
		);

		OpenGLCommandBuffer& CopyTextureToBuffer(
			OpenGLResource const& src_texture,
			OpenGLResource const& dest_buffer,
			CmdCopyTextureToBuffer::Subresource const& src_subresource,
			CmdCopyTextureToBuffer::Offset3D const& src_offset,
			std::size_t dest_offset,
			CmdCopyTextureToBuffer::Extent3D const& extent
		);
		
		OpenGLCommandBuffer& CopyBufferToTexture(
			OpenGLResource const& src_buffer,
			OpenGLResource const& dest_texture,
			std::size_t src_offset,
			CmdCopyBufferToTexture::Subresource const& dest_subresource,
			CmdCopyBufferToTexture::Offset3D const& dest_offset,
			CmdCopyBufferToTexture::Extent3D const& extent
		);
		
		OpenGLCommandBuffer& CopyBufferToBuffer(
			OpenGLResource const& src_buffer,
			OpenGLResource const& dest_buffer,
			std::size_t src_offset,
			std::size_t dest_offset,
			std::size_t size
		);
		
		OpenGLCommandBuffer& CopyTextureToTexture(
			OpenGLResource const& src_texture,
			OpenGLResource const& dest_texture,
			CmdCopyTextureToTexture::Subresource const& src_subresource,
			CmdCopyTextureToTexture::Subresource const& dest_subresource,
			CmdCopyTextureToTexture::Offset3D const& src_offset,
			CmdCopyTextureToTexture::Offset3D const& dest_offset,
			CmdCopyTextureToTexture::Extent3D const& extent
		);

		OpenGLCommandBuffer& BindIndexBuffer(
			OpenGLResource const& index_buffer
		);
		
		OpenGLCommandBuffer& ExecuteIndirect(
			OpenGLResource const& argument_buffer,
			std::size_t offset_into_buffer,
			std::size_t draw_count
		);
		
		OpenGLCommandBuffer& ExecuteIndirectIndexed(
			OpenGLResource const& argument_buffer,
			std::size_t offset_into_buffer,
			std::size_t draw_count
		);
		
		OpenGLCommandBuffer& DispatchIndirect(
			OpenGLResource const& argument_buffer,
			std::size_t offset_into_buffer,
			std::size_t block_size_x,
			std::size_t block_size_y,
			std::size_t block_size_z
		);
		

	};

} // namespace fyuu_rhi::opengl

#endif