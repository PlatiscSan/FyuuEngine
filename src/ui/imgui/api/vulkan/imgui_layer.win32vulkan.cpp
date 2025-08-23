module;
#ifdef WIN32
#include <Windows.h>
#endif // WIN32

module imgui_layer:win32vulkan;
import std;
import <imgui.h>;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

namespace ui::imgui::api::vulkan {
	
	boost::uuids::uuid Win32VulkanIMGUILayer::InitializeImGUI(
		platform::Win32Window* window,
		graphics::api::vulkan::Win32VulkanRenderDevice* device,
		Win32VulkanIMGUILayer* layer
	) {
		ImGui_ImplWin32_EnableDpiAwareness();
		float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(main_scale);
		ImGui_ImplWin32_Init(window->GetHWND());
		
		auto const& core = device->Core();

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = *(core.instance);
		init_info.PhysicalDevice = core.physical_device;
		init_info.Device = *(core.logical_device);
		init_info.QueueFamily = *core.graphics_queue_family;
		init_info.Queue = core.graphics_queue;
		init_info.PipelineCache = nullptr;
		init_info.DescriptorPool = *layer->m_descriptor_pool;
		init_info.RenderPass = *device->RenderPass();
		init_info.Subpass = 0;
		init_info.MinImageCount = 2;
		init_info.ImageCount = static_cast<std::uint32_t>(device->FrameCount());
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = reinterpret_cast<VkAllocationCallbacks const*>(core.allocator);
		
		ImGui_ImplVulkan_Init(&init_info);
		return window->AttachMsgProcessor(ImGui_ImplWin32_WndProcHandler);
	}

	Win32VulkanIMGUILayer::Win32VulkanIMGUILayer(
		platform::IWindow& window, 
		graphics::BaseRenderDevice& device, 
		ImVec4 const& clear_color
	) : BaseIMGUILayer(window, device, clear_color),
		m_descriptor_pool(
			static_cast<graphics::api::vulkan::Win32VulkanRenderDevice*>(m_device)->CreateDescriptorPool(
				vk::DescriptorType::eCombinedImageSampler,
				1
			)
		),
		m_layer_uuid(
			Win32VulkanIMGUILayer::InitializeImGUI(
				static_cast<platform::Win32Window*>(m_window),
				static_cast<graphics::api::vulkan::Win32VulkanRenderDevice*>(m_device),
				this
			)
		) {
	}

	Win32VulkanIMGUILayer::~Win32VulkanIMGUILayer() noexcept {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplWin32_Shutdown();
	}

	void Win32VulkanIMGUILayer::BeginFrame() {
		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplWin32_NewFrame();
		BaseIMGUILayer::BeginFrame();
	}

	void Win32VulkanIMGUILayer::Render() {

		auto device = static_cast<graphics::api::vulkan::Win32VulkanRenderDevice*>(m_device);
		auto& command_object = static_cast<graphics::api::vulkan::VulkanCommandObject&>(device->AcquireCommandObject());
		auto& command_buffer = command_object.CommandBuffer();

		BaseIMGUILayer::Render();
		auto [width, height] = m_window->GetWidthAndHeight();
		m_device->SetViewport(0, 0, width, height);
		m_device->Clear(m_clear_color.x, m_clear_color.y, m_clear_color.z, m_clear_color.w);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *command_buffer);

	}

}