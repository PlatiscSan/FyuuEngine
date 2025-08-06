module;
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <imgui.h>

#ifdef WIN32
#include <Windows.h>
#include <wrl/client.h>
#endif

module imgui_layer:d3d12;
import layer;
import graphics;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

namespace ui::imgui::api::d3d12 {
#ifdef WIN32
	boost::uuids::uuid D3D12ImGUILayer::InitializeImGUI(
		D3D12ImGUILayer* layer,
		platform::Win32Window* window,
		graphics::api::d3d12::D3D12RenderDevice* device
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
		init_info.UserData = device;
		// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
		// (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
		init_info.SrvDescriptorHeap = device->GetSRVHeap().Get();
		init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) {
			auto device = static_cast<graphics::api::d3d12::D3D12RenderDevice*>(info->UserData);
			auto [cpu_handle, gpu_handle] = device->AcquireDescriptorHandle();
			*out_cpu_handle = cpu_handle;
			*out_gpu_handle = gpu_handle;
			};
		init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) {
			auto device = static_cast<graphics::api::d3d12::D3D12RenderDevice*>(info->UserData);
			graphics::api::d3d12::DescriptorHandle handle{ cpu_handle , gpu_handle };
			device->ReturnDescriptorHandle(handle);
			};

		ImGui_ImplDX12_Init(&init_info);

		return window->AttachMsgProcessor(ImGui_ImplWin32_WndProcHandler);

	}

	D3D12ImGUILayer::D3D12ImGUILayer(platform::Win32Window& window, graphics::api::d3d12::D3D12RenderDevice& device)
		: m_window(&window), 
		m_device(&device), 
		m_layer_uuid(D3D12ImGUILayer::InitializeImGUI(this, m_window, m_device)) {

	}

	D3D12ImGUILayer::~D3D12ImGUILayer() noexcept {
		m_window->DetachMsgProcessor(m_layer_uuid);
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	void D3D12ImGUILayer::SetClearColor(float r, float g, float b, float a) {
		m_clear_color = ImVec4(r, g, b, a);
	}

	void D3D12ImGUILayer::BeginFrame() {
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void D3D12ImGUILayer::EndFrame() {

	}

	void D3D12ImGUILayer::Update() {
		ImGuiIO& io = ImGui::GetIO();

		if (m_show_demo_window)
			ImGui::ShowDemoWindow(&m_show_demo_window);

		{
			static float f = 0.0f;
			static int counter = 0;
			ImGui::Begin("Hello, world!");
			ImGui::Checkbox("Demo Window", &m_show_demo_window);
			ImGui::Checkbox("Another Window", &m_show_another_window);
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
			ImGui::ColorEdit3("clear color", (float*)&m_clear_color);
			if (ImGui::Button("Button"))
				counter++;
			ImGui::Text("counter = %d", counter);
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		if (m_show_another_window) {
			ImGui::Begin("Another Window", &m_show_another_window);
			if (ImGui::Button("Close Me"))
				m_show_another_window = false;
			ImGui::End();
		}
		
		m_device->SetClearColor(m_clear_color.x, m_clear_color.y, m_clear_color.z, m_clear_color.w);

	}

	void D3D12ImGUILayer::Render() {
		ImGui::Render();
		auto command_list = m_device->GetCommandList();
		ID3D12DescriptorHeap* heaps[] = { m_device->GetSRVHeap().Get() };
		command_list->SetDescriptorHeaps(1u, heaps);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command_list.Get());

	}
#endif // WIN32
}