module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef WIN32
#include <Windows.h>
#include <wrl/client.h>
#endif

module imgui_layer:d3d12;
import layer;
import graphics;
import <imgui.h>;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

namespace ui::imgui::api::d3d12 {
#ifdef WIN32
	boost::uuids::uuid D3D12IMGUILayer::InitializeImGUI(
		platform::Win32Window* window,
		D3D12IMGUILayer* imgui_layer
	) {

		auto device = static_cast<graphics::api::d3d12::D3D12RenderDevice*>(imgui_layer->m_device);

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
		//ImGui::StyleColorsLight();

		// Setup scaling

		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
		// style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

		// Setup Platform/Renderer backends
		ImGui_ImplWin32_Init(window->GetHWND());

		ImGui_ImplDX12_InitInfo init_info = {};
		init_info.Device = device->GetD3D12Device().Get();
		init_info.CommandQueue = device->GetCommandQueue().Get();
		init_info.NumFramesInFlight = static_cast<int>(device->FrameCount());
		init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
		init_info.UserData = imgui_layer;
		// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
		// (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
		init_info.SrvDescriptorHeap = imgui_layer->m_descriptor_pool.GetDescriptorHeap().Get();
		init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) {
			
			auto imgui_layer = static_cast<D3D12IMGUILayer*>(info->UserData);
			auto handle = imgui_layer->m_descriptor_pool.AcquireHandle();
			auto [cpu_handle, gpu_handle] = handle.Get();

			*out_cpu_handle = cpu_handle;
			*out_gpu_handle = gpu_handle;

			imgui_layer->m_allocated_handle.emplace_back(std::move(handle));

			};

		init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) {
			
			auto imgui_layer = static_cast<D3D12IMGUILayer*>(info->UserData);
			std::erase_if(
				imgui_layer->m_allocated_handle,
				[cpu_handle, gpu_handle](graphics::api::d3d12::DescriptorResourcePool::UniqueDescriptorHandle const& handle) {
					auto [allocated_cpu_handle, allocated_gpu_handle] = handle.Get();
					return allocated_cpu_handle.ptr == cpu_handle.ptr &&
						allocated_gpu_handle.ptr == gpu_handle.ptr;
				}
			);

			};

		ImGui_ImplDX12_Init(&init_info);

		return window->AttachMsgProcessor(ImGui_ImplWin32_WndProcHandler);

	}

	D3D12IMGUILayer::D3D12IMGUILayer(
		platform::IWindow& window,
		graphics::BaseRenderDevice& device,
		ImVec4 const& clear_color
	) : BaseIMGUILayer(window, device, clear_color),
		m_descriptor_pool(
			static_cast<graphics::api::d3d12::D3D12RenderDevice&>(device).CreateDescriptorResourcePool(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				128u
			)
		),
		m_allocated_handle(),
		m_layer_uuid(
			D3D12IMGUILayer::InitializeImGUI(
				static_cast<platform::Win32Window*>(m_window), 
				this
			)
		) {

	}

	D3D12IMGUILayer::~D3D12IMGUILayer() noexcept {
		m_allocated_handle.clear();
		static_cast<platform::Win32Window*>(m_window)->DetachMsgProcessor(m_layer_uuid);
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
	}

	void D3D12IMGUILayer::BeginFrame() {
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		BaseIMGUILayer::BeginFrame();
	}

	void D3D12IMGUILayer::Render() {
		BaseIMGUILayer::Render();
		auto device = static_cast<graphics::api::d3d12::D3D12RenderDevice*>(m_device);
		auto& command_object = static_cast<graphics::api::d3d12::D3D12CommandObject&>(device->AcquireCommandObject());
		auto command_list = command_object.GetCommandList();
		command_list->SetDescriptorHeaps(1, m_descriptor_pool.GetDescriptorHeap().GetAddressOf());
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command_list.Get());
	}

#endif // WIN32
}