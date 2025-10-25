module;
#include <fyuu_application.h>

export module adapter:capplication;
import :clogger;

namespace fyuu_engine::application {

	export class CApplicationAdapter final 
		: public IApplication {
	private:
		Fyuu_IApplication* m_app;
		std::optional<CLoggerAdapter> m_logger;

	public:
		CApplicationAdapter(Fyuu_IApplication* app);

		/// @brief	engine calls this function to bind your own logger
		/// @return logger interface you provide, nullptr for default logger
		ILogger* CustomLogger(LogLevel level, bool write_to_file, std::size_t max_size) override;

		/// @brief	engine calls this function to get launch configuration and then initialize.
		/// @return launch configuration
		ApplicationConfig GetConfig() const override;

		/// @brief	Handles the main window close event.
		void OnClose() override;

		/// @brief	Handles actions to be taken when main window resize event occurs.
		/// @param	width The new width after resizing, in pixels.
		/// @param	height The new height after resizing, in pixels.
		void OnResize(std::uint32_t width, std::uint32_t height) override;

		/// @brief		Handles the event when a key is pressed down.
		/// @param key	The code of the key that was pressed.
		void OnKeyDown(int key) override;

		/// @brief		Handles the event when a key is released.
		/// @param key	The code of the key that was released.
		void OnKeyUp(int key) override;

		/// @brief		Handles the event when a key is held down and repeats.
		/// @param key	The code of the key that is being repeated.
		void OnKeyRepeat(int key) override;

		/// @brief		Handles mouse movement events at the specified coordinates.
		/// @param x	The x-coordinate of the mouse pointer.
		/// @param y	The y-coordinate of the mouse pointer.
		void OnMouseMove(double x, double y) override;

		/// @brief			Handles the event when a mouse button is pressed.
		/// @param button	The mouse button that was pressed.
		/// @param x		The x-coordinate of the mouse pointer.
		/// @param y		The y-coordinate of the mouse pointer.
		void OnMouseButtonDown(Button button, double x, double y) override;

		/// @brief			Handles the event when a mouse button is released.
		/// @param button	The mouse button that was released.
		/// @param x		The x-coordinate of the mouse pointer.
		/// @param y		The y-coordinate of the mouse pointer.
		void OnMouseButtonUp(Button button, double x, double y) override;

		/// @brief			called at each frame, you can update resource here
		/// @param id		identify in which thread this function is called
		/// @param renderer renderer interface
		void OnUpdate(std::thread::id const& id, Renderer& renderer) override;

		/// @brief					called at each frame, commands should be recorded and then submitted to renderer.
		/// @param id				identify in which thread this function is called.
		/// @param command_object	command object interface
		void OnRender(std::thread::id const& id, CommandObject& command_object) override;

	};

}