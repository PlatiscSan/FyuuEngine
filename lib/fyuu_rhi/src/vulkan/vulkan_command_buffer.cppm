/* vulkan_command_buffer.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <atomic>
#include <thread>
#include <shared_mutex>
#include <condition_variable>
#include <vector>
#include <string_view>
#include <optional>
#include <span>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:vulkan_command_buffer;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import vulkan;
import :types;
import :vulkan_common;
import :vulkan_types;
import :vulkan_queue_allocator;
import :synchronization;

namespace fyuu_rhi::vulkan {

	export class VulkanCommandBuffer
		: public PolymorphicCommandBufferBase,
		public VulkanCommon<VulkanCommandBuffer> {
	private:
		std::optional<std::size_t> m_logical_device_id;
		std::shared_ptr<vk::detail::DispatchLoaderDynamic> m_dispatcher;
		vk::UniqueCommandPool m_pool;
		vk::UniqueCommandBuffer m_impl;
		std::vector<UniqueCommand> m_commands;
		SynchronizationObject m_sync;
		bool m_recording_flag;

	public:
		VulkanCommandBuffer(
			VulkanLogicalDevice const& logical_device,
			CommandObjectType type,
			vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary,
			std::string_view debug_name = {}
		);

		VulkanCommandBuffer& BeginRecording();
		void EndRecording();
		void Reset();

		VulkanCommandBuffer& SetFrame(
			VulkanResource const& frame, 
			VulkanView const& frame_view
		);

		VulkanCommandBuffer& BeginRendering(
			std::span<VulkanResource const* const> rts, 
			std::span<VulkanView const* const> rtvs, 
			std::span<LoadOp const> rt_loads, 
			std::span<StoreOp const> rt_stores, 
			std::span<std::array<float, 4u> const> rt_clears
		);

		VulkanCommandBuffer& BeginRendering(
			std::span<VulkanResource const* const> rts, 
			std::span<VulkanView const* const> rtvs, 
			std::span<LoadOp const> rt_loads,
			std::span<StoreOp const> rt_stores,
			std::span<std::array<float, 4u> const> rt_clears,
			VulkanResource const& ds, 
			VulkanView const& dsv, 
			LoadOp depth_load,
			StoreOp depth_store,
			float depth_clear,
			LoadOp stencil_load,
			StoreOp stencil_store,
			std::uint8_t stencil_clear
		);

		VulkanCommandBuffer& EndRendering();

		VulkanCommandBuffer& SetViewports(std::span<CmdSetViewports::Viewport const> viewports);

		VulkanCommandBuffer& SetScissors(std::span<CmdSetScissors::Scissor const> scissors);

		VulkanCommandBuffer& Execute(VulkanGPUExecutable const& gpu_exec);

		VulkanCommandBuffer& DispatchComputingTask(
			std::size_t threads_x,
			std::size_t threads_y,
			std::size_t threads_z,
			std::size_t threads_per_thread_group_x = 1u,
			std::size_t threads_per_thread_group_y = 1u,
			std::size_t threads_per_thread_group_z = 1u
		);

		
		VulkanCommandBuffer& BindVertexBuffers(std::span<VulkanResource const* const> vertex_buffers, std::span<std::size_t const> wheres, std::span<std::size_t const> offsets, std::span<std::size_t const> strides);

		VulkanCommandBuffer& DrawInstanced(
			std::size_t vertices,
			std::size_t instances = 1u,
			std::size_t start_vertex = 0u,
			std::size_t first_instance = 0u
		);

		VulkanCommandBuffer& DrawIndexed(
			std::size_t indices,
			std::size_t instances = 1u,
			std::size_t first_index = 0u,
			std::size_t start_vertex = 0u,
			std::size_t first_instance = 0u
		);

		VulkanCommandBuffer& BindBuffer(
			ShaderStage stages,
			std::size_t where,
			VulkanResource const& buffer,
			std::size_t offset = 0u,
			std::size_t ns = 0
		);

		VulkanCommandBuffer& BindBuffer(
			ShaderStage stages,
			std::size_t where,
			VulkanResource const& buffer,
			VulkanView const& texel_view,
			std::size_t offset = 0u,
			std::size_t ns = 0
		);

		VulkanCommandBuffer& PushData(
			ShaderStage stages,
			std::size_t where,
			std::span<std::byte const> bytes,
			std::size_t offset = 0u,
			std::size_t ns = 0u
		);

		VulkanCommandBuffer& BindSampler(
			ShaderStage stages,
			std::size_t where,
			VulkanSampler const& sampler,
			std::size_t ns = 0u
		);

		VulkanCommandBuffer& BindTexture(
			ShaderStage stages,
			std::size_t where,
			VulkanResource const& texture,
			VulkanView const& view,
			std::size_t ns = 0u
		);

		VulkanCommandBuffer& CopyTextureToBuffer(
			VulkanResource const& src_texture,
			VulkanResource const& dest_buffer,
			CmdCopyTextureToBuffer::Subresource const& src_subresource,
			CmdCopyTextureToBuffer::Offset3D const& src_offset,
			std::size_t dest_offset,
			CmdCopyTextureToBuffer::Extent3D const& extent
		);
		
		VulkanCommandBuffer& CopyBufferToTexture(
			VulkanResource const& src_buffer,
			VulkanResource const& dest_texture,
			std::size_t src_offset,
			CmdCopyBufferToTexture::Subresource const& dest_subresource,
			CmdCopyBufferToTexture::Offset3D const& dest_offset,
			CmdCopyBufferToTexture::Extent3D const& extent
		);
		
		VulkanCommandBuffer& CopyBufferToBuffer(
			VulkanResource const& src_buffer,
			VulkanResource const& dest_buffer,
			std::size_t src_offset,
			std::size_t dest_offset,
			std::size_t size
		);
		
		VulkanCommandBuffer& CopyTextureToTexture(
			VulkanResource const& src_texture,
			VulkanResource const& dest_texture,
			CmdCopyTextureToTexture::Subresource const& src_subresource,
			CmdCopyTextureToTexture::Subresource const& dest_subresource,
			CmdCopyTextureToTexture::Offset3D const& src_offset,
			CmdCopyTextureToTexture::Offset3D const& dest_offset,
			CmdCopyTextureToTexture::Extent3D const& extent
		);

		VulkanCommandBuffer& BindIndexBuffer(
			VulkanResource const& index_buffer
		);
		
		VulkanCommandBuffer& ExecuteIndirect(
			VulkanResource const& argument_buffer,
			std::size_t offset_into_buffer,
			std::size_t draw_count
		);
		
		VulkanCommandBuffer& ExecuteIndirectIndexed(
			VulkanResource const& argument_buffer,
			std::size_t offset_into_buffer,
			std::size_t draw_count
		);
		
		VulkanCommandBuffer& DispatchIndirect(
			VulkanResource const& argument_buffer,
			std::size_t offset_into_buffer,
			std::size_t block_size_x,
			std::size_t block_size_y,
			std::size_t block_size_z
		);
		
		vk::CommandBuffer GetNative() const noexcept;

	};
} // namespace fyuu_rhi::vulkan

#endif // !defined(__APPLE__)