module imgui_layer:base;
import std;

namespace ui::imgui {

	BaseIMGUILayer::BaseIMGUILayer(
		platform::IWindow& window, 
		graphics::BaseRenderDevice& device, 
		ImVec4 const& clear_color
	) : m_window(&window),
		m_device(&device),
		m_clear_color(clear_color) {

	}

	BaseIMGUILayer::~BaseIMGUILayer() noexcept {
		ImGui::DestroyContext();
	}

	void BaseIMGUILayer::BeginFrame() {
		ImGui::NewFrame();
	}

	void BaseIMGUILayer::EndFrame() {
	}

	void BaseIMGUILayer::Update() {

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

		m_device->Clear(m_clear_color.x, m_clear_color.y, m_clear_color.z, m_clear_color.w);

	}

	void BaseIMGUILayer::Render() {
		ImGui::Render();
	}

}