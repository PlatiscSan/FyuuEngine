module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef WIN32
#include <Windows.h>
#endif // WIN32

export module imgui_layer:win32vulkan;
export import :base;
import std;

#ifdef WIN32
export import <imgui_impl_win32.h>;
#endif // WIN32
import <imgui_impl_vulkan.h>;
import window;

namespace ui::imgui::api::vulkan {
	export class Win32VulkanIMGUILayer final : public BaseIMGUILayer {
	public:
		vk::UniqueDescriptorPool m_descriptor_pool;
		boost::uuids::uuid m_layer_uuid;
		static boost::uuids::uuid InitializeImGUI(
			platform::Win32Window* window,
			graphics::api::vulkan::Win32VulkanRenderDevice* device,
			Win32VulkanIMGUILayer* layer
		);
	public:
		Win32VulkanIMGUILayer(
			platform::IWindow& window,
			graphics::BaseRenderDevice& device,
			ImVec4 const& clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f)
		);
		~Win32VulkanIMGUILayer() noexcept override;
		void BeginFrame() override;
		void Render() override;
	};
}