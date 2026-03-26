module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <array>
#include <span>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:command_buffer;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import plastic.sealed_polymorphism;
import :common;
import :logical_device;
import :resource;
import :view;
import :gpu_executable;
import :sampler;

namespace fyuu_rhi {

	export class CommandBuffer {
	private:
		UniqueCommandBuffer m_impl;

	public:
		CommandBuffer(LogicalDevice const& logical_device, CommandObjectType buffer_type);

		void BeginRecording();
		void EndRecording();
		void Reset();

		CommandBuffer& SetFrame(
			PolymorphicResourceBase const* frame, 
			PolymorphicViewBase const* frame_view
		);

		CommandBuffer& BeginRendering(
			std::span<PolymorphicResourceBase const* const> rts, 
			std::span<PolymorphicViewBase const* const> rtvs, 
			std::span<LoadOp const> rt_loads, 
			std::span<StoreOp const> rt_stores, 
			std::span<std::array<float, 4u> const> rt_clears
		);

		CommandBuffer& BeginRendering(
			std::span<PolymorphicResourceBase const* const> rts, 
			std::span<PolymorphicViewBase const* const> rtvs, 
			std::span<LoadOp const> rt_loads,
			std::span<StoreOp const> rt_stores,
			std::span<std::array<float, 4u> const> rt_clears,
			Resource const& ds, 
			View const& dsv, 
			LoadOp depth_load,
			StoreOp depth_store,
			float depth_clear,
			LoadOp stencil_load,
			StoreOp stencil_store,
			std::uint8_t stencil_clear
		);

		CommandBuffer& EndRendering();

		CommandBuffer& SetViewports(std::span<CmdSetViewports::Viewport const> viewports);

		CommandBuffer& SetScissors(std::span<CmdSetScissors::Scissor const> scissors);

		CommandBuffer& Execute(GPUExecutable const& gpu_exec);

		CommandBuffer& DispatchComputingTask(
			std::size_t threads_x,
			std::size_t threads_y,
			std::size_t threads_z,
			std::size_t threads_per_thread_group_x = 1u,
			std::size_t threads_per_thread_group_y = 1u,
			std::size_t threads_per_thread_group_z = 1u
		);

		
		CommandBuffer& BindVertexBuffers(std::span<PolymorphicResourceBase const* const> vertex_buffers, std::span<std::size_t const> wheres, std::span<std::size_t const> offsets, std::span<std::size_t const> strides);

		CommandBuffer& DrawInstanced(
			std::size_t vertices,
			std::size_t instances = 1u,
			std::size_t start_vertex = 0u,
			std::size_t first_instance = 0u
		);

		CommandBuffer& DrawIndexed(
			std::size_t indices,
			std::size_t instances = 1u,
			std::size_t first_index = 0u,
			std::size_t start_vertex = 0u,
			std::size_t first_instance = 0u
		);

		CommandBuffer& BindBuffer(
			ShaderStage stages,
			std::size_t where,
			Resource const& buffer,
			std::size_t offset = 0u,
			std::size_t ns = 0
		);

		CommandBuffer& BindBuffer(
			ShaderStage stages,
			std::size_t where,
			Resource const& buffer,
			View const& texel_view,
			std::size_t offset = 0u,
			std::size_t ns = 0
		);

		CommandBuffer& PushData(
			ShaderStage stages,
			std::size_t where,
			std::span<std::byte const> bytes,
			std::size_t offset = 0u, 
			std::size_t ns = 0
		);

		CommandBuffer& BindSampler(
			ShaderStage stages,
			std::size_t where,
			Sampler const& sampler,
			std::size_t ns = 0u
		);

		CommandBuffer& BindTexture(
			ShaderStage stages,
			std::size_t where,
			Resource const& texture,
			View const& view,
			std::size_t ns = 0u
		);

		CommandBuffer& CopyTextureToBuffer(
			Resource const& src_texture,
			Resource const& dest_buffer,
			CmdCopyTextureToBuffer::Subresource const& src_subresource,
			CmdCopyTextureToBuffer::Offset3D const& src_offset,
			std::size_t dest_offset,
			CmdCopyTextureToBuffer::Extent3D const& extent
		);
		
		CommandBuffer& CopyBufferToTexture(
			Resource const& src_buffer,
			Resource const& dest_texture,
			std::size_t src_offset,
			CmdCopyBufferToTexture::Subresource const& dest_subresource,
			CmdCopyBufferToTexture::Offset3D const& dest_offset,
			CmdCopyBufferToTexture::Extent3D const& extent
		);
		
		CommandBuffer& CopyBufferToBuffer(
			Resource const& src_buffer,
			Resource const& dest_buffer,
			std::size_t src_offset,
			std::size_t dest_offset,
			std::size_t size
		);
		
		CommandBuffer& CopyTextureToTexture(
			Resource const& src_texture,
			Resource const& dest_texture,
			CmdCopyTextureToTexture::Subresource const& src_subresource,
			CmdCopyTextureToTexture::Subresource const& dest_subresource,
			CmdCopyTextureToTexture::Offset3D const& src_offset,
			CmdCopyTextureToTexture::Offset3D const& dest_offset,
			CmdCopyTextureToTexture::Extent3D const& extent
		);

		CommandBuffer& BindIndexBuffer(
			Resource const& index_buffer
		);
		
		CommandBuffer& ExecuteIndirect(
			Resource const& argument_buffer,
			std::size_t offset_into_buffer,
			std::size_t draw_count
		);
		
		CommandBuffer& ExecuteIndirectIndexed(
			Resource const& argument_buffer,
			std::size_t offset_into_buffer,
			std::size_t draw_count
		);
		
		CommandBuffer& DispatchIndirect(
			Resource const& argument_buffer,
			std::size_t offset_into_buffer,
			std::size_t block_size_x,
			std::size_t block_size_y,
			std::size_t block_size_z
		);
	
	};
	
}