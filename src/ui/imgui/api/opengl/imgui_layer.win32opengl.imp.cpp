module;
#include <GL/glcorearb.h>
#ifdef WIN32
#include <GL/wglext.h>
#endif // WIN32

module imgui_layer:win32opengl;
import window;
import <imgui.h>;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

namespace ui::imgui::api::opengl {
#ifdef WIN32

	boost::uuids::uuid Win32OpenGLIMGUILayer::InitializeImGUI(
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

	Win32OpenGLIMGUILayer::Win32OpenGLIMGUILayer(
		platform::IWindow& window,
		graphics::BaseRenderDevice& device,
		ImVec4 const& clear_color
	) : BaseIMGUILayer(window, device, clear_color),
		m_layer_uuid(
			Win32OpenGLIMGUILayer::InitializeImGUI(
				static_cast<platform::Win32Window*>(m_window), 
				static_cast<graphics::api::opengl::Win32OpenGLRenderDevice*>(m_device)
			)
		) {

	}

	Win32OpenGLIMGUILayer::~Win32OpenGLIMGUILayer() noexcept {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplWin32_Shutdown();
	}

	void Win32OpenGLIMGUILayer::BeginFrame() {
		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplWin32_NewFrame();
		BaseIMGUILayer::BeginFrame();
	}

	void Win32OpenGLIMGUILayer::Render() {

		BaseIMGUILayer::Render();
		auto [width, height] = m_window->GetWidthAndHeight();
		m_device->SetViewport(0, 0, width, height);
		m_device->Clear(m_clear_color.x, m_clear_color.y, m_clear_color.z, m_clear_color.w);
		auto& command_object = static_cast<graphics::api::opengl::OpenGLCommandObject&>(m_device->AcquireCommandObject());
		command_object.SubmitCommand(
			[]() {
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			}
		);

	}

#endif // WIN32
}
