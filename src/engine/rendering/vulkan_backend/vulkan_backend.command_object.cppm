export module vulkan_backend:command_object;
export import pointer_wrapper;
import rendering;
import std;
import vulkan_hpp;
export import :buffer;

namespace fyuu_engine::vulkan {

	export struct VulkanFrameBuffer {
		vk::RenderPass vk_render_pass;
		vk::Framebuffer vk_frame_buffer;
		vk::Image vk_image;
		vk::Rect2D area;
	};

	export struct VulkanCommandObjectTraits {

		using PipelineStateObjectType = vk::UniquePipeline;
		using OutputTargetType = VulkanFrameBuffer;
		using BufferType = VulkanBufferAllocation;
		using TextureType = std::monostate;

	};

	export struct VulkanCommandBufferReady {
		vk::UniqueCommandBuffer& buffer;
	};

	export struct VulkanFrameIndexRequest {
		std::size_t& current_frame_index;
	};

	export class VulkanCommandObject
		: public core::BaseCommandObject<VulkanCommandObject, VulkanCommandObjectTraits> {
		friend class core::BaseCommandObject<VulkanCommandObject, VulkanCommandObjectTraits>;
	private:
		util::PointerWrapper<concurrency::SyncMessageBus> m_message_bus;
		vk::Device m_logical_device;
		vk::UniqueCommandPool m_command_pool;
		std::vector<vk::UniqueCommandBuffer> m_command_buffers;
		std::size_t m_current_frame_index;

		void BeginRecordingImpl();
		void EndRecordingImpl();
		void ResetImpl();
		void SetViewportImpl(core::Viewport const& viewport);
		void SetScissorRectImpl(core::Rect const& rect);
		void BarrierImpl(VulkanBufferAllocation const& buffer, core::ResourceBarrierDesc const& desc);
		void BarrierImpl(VulkanFrameBuffer const& render_pass, core::ResourceBarrierDesc const& desc);
		void BarrierImpl(std::monostate const& texture, core::ResourceBarrierDesc const& desc);
		void BeginRenderPassImpl(VulkanFrameBuffer const& render_pass, std::span<float> clear_value);
		void EndRenderPassImpl(VulkanFrameBuffer const& render_pass);
		void BindVertexBufferImpl(VulkanBufferAllocation const& vertex_buffer, core::VertexDesc const& desc);
		void SetPrimitiveTopologyImpl(core::PrimitiveTopology primitive_topology);
		void DrawImpl(
			std::uint32_t index_count, std::uint32_t instance_count,
			std::uint32_t start_index, std::int32_t base_vertex,
			std::uint32_t start_instance
		);
		void ClearImpl(VulkanFrameBuffer const& render_pass, std::span<float> rgba, core::Rect const& rect);
		void CopyImpl(VulkanBufferAllocation const& src, VulkanBufferAllocation const& dest);

	public:
		VulkanCommandObject(
			util::PointerWrapper<concurrency::SyncMessageBus> const& message_bus,
			std::uint32_t frame_count,
			vk::Device const& logical_device, 
			std::uint32_t queue_family, 
			vk::AllocationCallbacks const* allocator = nullptr
		);
		
	};

}
