module adapter:capplication;
import std;

namespace fyuu_engine::application {
	CApplicationAdapter::CApplicationAdapter(Fyuu_IApplication* app)
		: m_app(app), m_logger(m_app->CustomLogger ? m_app->CustomLogger() : nullptr) {
	}

	ILogger* CApplicationAdapter::CustomLogger() {
		return m_logger ? &m_logger : IApplication::CustomLogger();
	}

	ApplicationConfig CApplicationAdapter::GetConfig() const {
		return m_app->GetConfig ? m_app->GetConfig() : IApplication::GetConfig();
	}

	void CApplicationAdapter::OnClose() {
		m_app->OnClose ? m_app->OnClose() : IApplication::OnClose();
	}

	void CApplicationAdapter::OnResize(std::uint32_t width, std::uint32_t height) {
		m_app->OnResize ? m_app->OnResize(width, height) : IApplication::OnResize(width, height);
	}

	void CApplicationAdapter::OnKeyDown(int key) {
		m_app->OnKeyDown ? m_app->OnKeyDown(key) : IApplication::OnKeyDown(key);
	}

	void CApplicationAdapter::OnKeyUp(int key) {
		m_app->OnKeyUp ? m_app->OnKeyUp(key) : IApplication::OnKeyUp(key);
	}

	void CApplicationAdapter::OnKeyRepeat(int key) {
		m_app->OnKeyRepeat ? m_app->OnKeyRepeat(key) : IApplication::OnKeyRepeat(key);
	}

	void CApplicationAdapter::OnMouseMove(double x, double y) {
		m_app->OnMouseMove ? m_app->OnMouseMove(x, y) : IApplication::OnMouseMove(x, y);
	}

	void CApplicationAdapter::OnMouseButtonDown(Button button, double x, double y) {
		m_app->OnMouseButtonDown ? m_app->OnMouseButtonDown(static_cast<Fyuu_Button>(button), x, y) : IApplication::OnMouseButtonDown(button, x, y);
	}

	void CApplicationAdapter::OnMouseButtonUp(Button button, double x, double y) {
		m_app->OnMouseButtonUp ? m_app->OnMouseButtonUp(static_cast<Fyuu_Button>(button), x, y) : IApplication::OnMouseButtonUp(button, x, y);
	}

	void CApplicationAdapter::OnUpdate(std::thread::id const& id, Renderer& renderer) {
		m_app->OnUpdate ? m_app->OnUpdate(std::hash<std::thread::id>()(id), renderer) : IApplication::OnUpdate(id, renderer);
	}

	void CApplicationAdapter::OnRender(std::thread::id const& id, CommandObject& command_object) {
		m_app->OnRender ? m_app->OnRender(std::hash<std::thread::id>()(id), command_object) : IApplication::OnRender(id, command_object);
	}

}