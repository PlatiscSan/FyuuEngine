#include <fyuu_application.h>

class HelloWorld final : public fyuu_engine::application::IApplication {
private:
	/// @brief			called at each frame, you can update resource here
	/// @param id		identify in which thread this function is called
	/// @param renderer renderer interface
	void OnUpdate(std::thread::id const& id, fyuu_engine::application::Renderer& renderer) {

	}

	/// @brief					called at each frame, commands should be recorded and then submitted to renderer.
	/// @param id				identify in which thread this function is called.
	/// @param command_object	command object interface
	void OnRender(std::thread::id const& id, fyuu_engine::application::CommandObject& command_object) {
		
		constexpr std::array color = { 0.0f, 0.2f, 0.4f, 1.0f };
		
		command_object.BeginRecording();
		command_object.BeginRenderPass(0.0f, 0.2f, 0.4f, 1.0f);
		command_object.EndRenderPass();
		command_object.EndRecording();

	}
};

int main(void) {
	HelloWorld hello_world;
	return fyuu_engine::application::RunApp(&hello_world);

	//Fyuu_IApplication hello_world{};
	//return Fyuu_RunApp(&hello_world);
}