#include <fyuu_application.h>

class Editor final : public fyuu_engine::application::IApplication {
private:
	std::thread::id m_main_thread_id;

	void OnMainThreadRender(fyuu_engine::application::CommandObject& command_object) {
		command_object.BeginRecording();
		command_object.BeginRenderPass(0.0f, 0.2f, 0.4f, 1.0f);
		command_object.EndRenderPass();
		command_object.EndRecording();
	}

	/// @brief			called at each frame, you can update resource here
	/// @param id		identify in which thread this function is called
	/// @param renderer renderer interface
	void OnUpdate(std::thread::id const& id, fyuu_engine::application::Renderer& renderer) {

	}

	/// @brief					called at each frame, commands should be recorded and then submitted to renderer.
	/// @param id				identify in which thread this function is called.
	/// @param command_object	command object interface
	void OnRender(std::thread::id const& id, fyuu_engine::application::CommandObject& command_object) {
		if (m_main_thread_id == id) {
			OnMainThreadRender(command_object);
		}
	}

public:
	Editor()
		: m_main_thread_id(std::this_thread::get_id()) {

	}
};

int main(void) {
	Editor editor;
	return fyuu_engine::application::RunApp(&editor);
}