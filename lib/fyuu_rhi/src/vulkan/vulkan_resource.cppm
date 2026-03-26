module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <optional>
#include <variant>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include <vma/vk_mem_alloc.h>
export module fyuu_rhi:vulkan_resource;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import vulkan;
import plastic.resource;
import :types;
import :vulkan_common;

namespace fyuu_rhi::vulkan {

	export struct VulkanResourceState {
		vk::PipelineStageFlags2 stage;
		vk::AccessFlags2 access;
		vk::ImageLayout layout;
	};

	export class VulkanResource
		: public PolymorphicResourceBase,
		public VulkanCommon<VulkanResource> {
	private:
		struct BufferHandle {
			vk::BufferCreateInfo buffer_info;
			vk::Buffer impl;
			VmaAllocation allocation;
			VmaAllocationInfo alloc_info;
			vk::Format format;
		};

		class BufferHandleGC {
		private:
			std::optional<std::size_t> m_logical_device_id;

		public:
			BufferHandleGC(VulkanLogicalDevice const& logical_device) noexcept;

			void operator()(BufferHandle& handle) noexcept;

		};

		using UniqueBufferHandle = plastic::utility::UniqueResource<BufferHandle, BufferHandleGC>;

		struct TextureHandle {
			vk::ImageCreateInfo texture_info;
			vk::Image impl;
			VmaAllocation allocation;
			VmaAllocationInfo alloc_info;
		};

		class TextureHandleGC {
		private:
			std::optional<std::size_t> m_logical_device_id;

		public:
			TextureHandleGC(VulkanLogicalDevice const& logical_device) noexcept;

			void operator()(TextureHandle& handle) noexcept;

		};

		using UniqueTextureHandle = plastic::utility::UniqueResource<TextureHandle, TextureHandleGC>;

		struct BackBuffer {
			vk::Image impl;	
			TextureSize size;
			vk::Format format;
		};

		VulkanResourceState m_state;
		std::variant<std::monostate, UniqueBufferHandle, UniqueTextureHandle, BackBuffer> m_handle;

	public:
		VulkanResource(VulkanLogicalDevice const& logical_device, BufferSize buffer_size, VideoMemoryType type, ResourceFlags flags);

		VulkanResource(VulkanLogicalDevice const& logical_device, TextureSize const& texture_size, VideoMemoryType type, ResourceFlags flags);

		VulkanResource(vk::Image back_buffer, vk::Format back_buffer_format, std::size_t width, std::size_t height);

		union Native {
			vk::Image texture;
			vk::Buffer buffer;

			Native()
				: texture(nullptr) {

			}

			Native(vk::Image texture_)
				: texture(texture_) {

			}

			Native(vk::Buffer buffer_)
				: buffer(buffer_) {

			}

		};
		
		Native GetNative() const noexcept;

		vk::Format GetFormat() const noexcept; 

		vk::ImageCreateInfo const* GetTextureDescription() const noexcept;

		vk::BufferCreateInfo const* GetBufferDescription() const noexcept;

		VulkanResourceState const& GetState() const noexcept;

		void SetState(VulkanResourceState const& state) noexcept;

		TextureSize GetTextureSize() const noexcept;

		vk::ImageAspectFlags GetAspectFlags() const noexcept;

		vk::ImageSubresourceRange WholeResourceRange() const noexcept;

	};


}
#endif // !defined(__APPLE__)