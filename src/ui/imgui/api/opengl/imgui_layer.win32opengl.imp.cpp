module;
#include <GL/glcorearb.h>
#ifdef WIN32
#include <GL/wglext.h>
#endif // WIN32
#include <imgui.h>

module imgui_layer:win32opengl;
import window;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

namespace ui::imgui::api::opengl {
#ifdef WIN32

	boost::uuids::uuid Win32OpenGLImGUILayer::InitializeImGUI(
		Win32OpenGLImGUILayer* layer, 
		platform::Win32Window* window, 
		graphics::api::opengl::Win32OpenGLRenderDevice* device
	) {

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// Setup Platform/Renderer backends
		ImGui_ImplWin32_InitForOpenGL(window->GetHWND());
		ImGui_ImplOpenGL3_Init();

		return window->AttachMsgProcessor(ImGui_ImplWin32_WndProcHandler);

	}

	Win32OpenGLImGUILayer::Win32OpenGLImGUILayer(platform::Win32Window& window, graphics::api::opengl::Win32OpenGLRenderDevice& device)
		: m_window(&window), 
		m_device(&device), 
		m_layer_uuid(Win32OpenGLImGUILayer::InitializeImGUI(this, m_window, m_device)) {

	}

	Win32OpenGLImGUILayer::~Win32OpenGLImGUILayer() noexcept {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	void Win32OpenGLImGUILayer::BeginFrame() {
		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void Win32OpenGLImGUILayer::EndFrame() {
	}

	void Win32OpenGLImGUILayer::Update() {

		ImGuiIO& io = ImGui::GetIO(); (void)io;

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

	}

	void Win32OpenGLImGUILayer::Render() {

		ImGui::Render();
		auto [width, height] = m_window->GetWidthAndHeight();
		m_device->SetViewport(0, 0, width, height);
		m_device->Clear(m_clear_color.x, m_clear_color.y, m_clear_color.z, m_clear_color.w);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	}

#endif // WIN32
}
