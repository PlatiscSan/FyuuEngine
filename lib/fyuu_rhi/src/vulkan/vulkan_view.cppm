/* vulkan_view.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <variant>
#endif
export module fyuu_rhi:vulkan_view;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import vulkan;
import plastic.registrable;
import plastic.resource;
import :vulkan_types;
import :enums;
import :vulkan_logical_device;
import :vulkan_common;

namespace fyuu_rhi::vulkan {

	export class VulkanView
		: public PolymorphicViewBase,
		public VulkanCommon<VulkanView> {
	private:
		struct GC {
			template <class T>
			void operator()(T&& t) {

			}
		};

		struct BufferView {
			vk::BufferViewCreateInfo info;
			vk::UniqueBufferView impl;
		};

		struct TextureView {
			vk::ImageViewCreateInfo info;
			vk::UniqueImageView impl;
		};

		using SharedBufferView = plastic::utility::SharedResource<BufferView, GC>;
		using SharedTextureView = plastic::utility::SharedResource<TextureView, GC>;
		using Handle = std::variant<std::monostate, SharedBufferView, SharedTextureView>;
		Handle m_handle;

	public:
		VulkanView(VulkanLogicalDevice const& logical_device, VulkanResource const& back_buffer, std::size_t index);

		VulkanView(VulkanLogicalDevice const& logical_device, VulkanResource const& resource, BufferSize buffer_size, BufferSize offset);

		VulkanView(VulkanLogicalDevice const& logical_device, VulkanResource const& resource, std::size_t base_mip_level, std::size_t level_count, std::size_t base_layer, std::size_t layer_count);

		union Native {
			vk::ImageView texture;
			vk::BufferView buffer;

			Native()
				: texture(nullptr) {

			}

			Native(vk::ImageView texture_)
				: texture(texture_) {

			}

			Native(vk::BufferView buffer_)
				: buffer(buffer_) {

			}

		};

		Native GetNative() const noexcept;

		vk::ImageViewCreateInfo const* GetTextureDescription() const noexcept;

		vk::BufferViewCreateInfo const* GetBufferDescription() const noexcept;

	};


}

#endif // !defined(__APPLE__)