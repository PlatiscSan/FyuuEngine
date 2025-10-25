#ifndef FYUU_APPLICATION_H
#define FYUU_APPLICATION_H

#include <fyuu_rendering.h>

#ifdef _WIN32
#include <Windows.h>
#include <tchar.h>
#endif // _WIN32

#ifdef __cplusplus
#include <optional>
#include <thread>
#include <filesystem>
#endif // __cplusplus

EXTERN_C{

	typedef enum Fyuu_Button { 
		FYUU_BUTTON_LEFT, 
		FYUU_BUTTON_RIGHT,
		FYUU_BUTTON_MIDDLE
	} Fyuu_Button;

	typedef struct Fyuu_ApplicationConfig {
		
		typedef enum Fyuu_ConfigFormat {
			FYUU_CONFIG_FORMAT_YAML = 0,
			FYUU_CONFIG_FORMAT_JSON = 1
		} Fyuu_ConfigFormat;

		char const* path;
		Fyuu_ConfigFormat format;

	} Fyuu_ApplicationConfig;

	typedef struct Fyuu_IApplication {
		typedef Fyuu_ILogger* (*Fyuu_CustomLoggerFuncPtr)(Fyuu_LogLevel level, bool write_to_file, size_t max_size);
		typedef Fyuu_ApplicationConfig(*Fyuu_GetConfigFuncPtr)();

		typedef void (*Fyuu_OnCloseFuncPtr)();
		typedef void (*Fyuu_OnResizeFuncPtr)(uint32_t width, uint32_t height);
		typedef void (*Fyuu_OnKeyDownFuncPtr)(int key);
		typedef void (*Fyuu_OnKeyUpFuncPtr)(int key);
		typedef void (*Fyuu_OnKeyRepeatFuncPtr)(int key);
		typedef void (*Fyuu_OnMouseMoveFuncPtr)(double x, double y);
		typedef void (*Fyuu_OnMouseButtonDownFuncPtr)(Fyuu_Button button, double x, double y);
		typedef void (*Fyuu_OnMouseButtonUpFuncPtr)(Fyuu_Button button, double x, double y);
		typedef void (*Fyuu_OnUpdateFuncPtr)(size_t, Fyuu_Renderer renderer);
		typedef void (*Fyuu_OnRenderFuncPtr)(size_t, Fyuu_CommandObject command_object);


		/// @brief					Creates or retrieves a custom logger configured with the specified log level and file options.
		/// @param level			The log level the logger should use (controls which message severities are recorded).
		/// @param write_to_file	If true, the logger writes output to a file; if false, it does not write to a file.
		/// @param max_size The		maximum size (in bytes) for the log file before rotation or truncation when file output is enabled.
		/// @return					A pointer to an ILogger instance configured with the given options.
		Fyuu_CustomLoggerFuncPtr CustomLogger;

		/// @brief	engine calls this function to get launch configuration and then initialize.
		/// @return launch configuration
		Fyuu_GetConfigFuncPtr GetConfig;

		/// @brief	Handles the main window close event.
		Fyuu_OnCloseFuncPtr OnClose;

		/// @brief	Handles actions to be taken when main window resize event occurs.
		/// @param	width The new width after resizing, in pixels.
		/// @param	height The new height after resizing, in pixels.
		Fyuu_OnResizeFuncPtr OnResize;

		/// @brief		Handles the event when a key is pressed down.
		/// @param key	The code of the key that was pressed.
		Fyuu_OnKeyDownFuncPtr OnKeyDown;

		/// @brief		Handles the event when a key is released.
		/// @param key	The code of the key that was released.
		Fyuu_OnKeyUpFuncPtr OnKeyUp;

		/// @brief		Handles the event when a key is held down and repeats.
		/// @param key	The code of the key that is being repeated.
		Fyuu_OnKeyRepeatFuncPtr OnKeyRepeat;

		/// @brief		Handles mouse movement events at the specified coordinates.
		/// @param x	The x-coordinate of the mouse pointer.
		/// @param y	The y-coordinate of the mouse pointer.
		Fyuu_OnMouseMoveFuncPtr OnMouseMove;

		/// @brief			Handles the event when a mouse button is pressed.
		/// @param button	The mouse button that was pressed.
		/// @param x		The x-coordinate of the mouse pointer.
		/// @param y		The y-coordinate of the mouse pointer.
		Fyuu_OnMouseButtonDownFuncPtr OnMouseButtonDown;

		/// @brief			Handles the event when a mouse button is released.
		/// @param button	The mouse button that was released.
		/// @param x		The x-coordinate of the mouse pointer.
		/// @param y		The y-coordinate of the mouse pointer.
		Fyuu_OnMouseButtonUpFuncPtr OnMouseButtonUp;

		/// @brief			called at each frame, you can update resource here
		/// @param id		identify in which thread this function is called
		/// @param renderer renderer interface
		Fyuu_OnUpdateFuncPtr OnUpdate;

		/// @brief					called at each frame, commands should be recorded and then submitted to renderer.
		/// @param id				identify in which thread this function is called.
		/// @param command_object	command object interface
		Fyuu_OnRenderFuncPtr OnRender;

	} Fyuu_IApplication;

#ifndef STATIC_LIB
	/// @brief Call this to launch app with your derived instance;
	DLL_API int DLL_CALL Fyuu_RunApp(Fyuu_IApplication* app) NO_EXCEPT;
#endif // !STATIC_LIB
	/// @brief Call this to stop the fyuu_engine instance.
	DLL_API void DLL_CALL Fyuu_RequestStop() NO_EXCEPT;
}


#ifdef __cplusplus

namespace fyuu_engine::application {

	enum class Button : std::uint8_t { 
		Left, 
		Right, 
		Middle 
	};

	struct ApplicationConfig {

		enum class ConfigFormat : std::uint8_t {
			YAML = 0,
			JSON = 1
		};

		std::filesystem::path path;
		ConfigFormat format = ConfigFormat::YAML;

		ApplicationConfig() = default;
		ApplicationConfig(Fyuu_ApplicationConfig const& c_config);

	};

	class DLL_CLASS IApplication {
	public:
		virtual ~IApplication() = default;

		/// @brief					Creates or retrieves a custom logger configured with the specified log level and file options.
		/// @param level			The log level the logger should use (controls which message severities are recorded).
		/// @param write_to_file	If true, the logger writes output to a file; if false, it does not write to a file.
		/// @param max_size The		maximum size (in bytes) for the log file before rotation or truncation when file output is enabled.
		/// @return					A pointer to an ILogger instance configured with the given options.
		virtual ILogger* CustomLogger(LogLevel level, bool write_to_file, std::size_t max_size);

		/// @brief	engine calls this function to get launch configuration and then initialize.
		/// @return launch configuration
		virtual ApplicationConfig GetConfig() const;

		/// @brief	Handles the main window close event.
		virtual void OnClose();

		/// @brief	Handles actions to be taken when main window resize event occurs.
		/// @param	width The new width after resizing, in pixels.
		/// @param	height The new height after resizing, in pixels.
		virtual void OnResize(std::uint32_t width, std::uint32_t height);

		/// @brief		Handles the event when a key is pressed down.
		/// @param key	The code of the key that was pressed.
		virtual void OnKeyDown(int key);

		/// @brief		Handles the event when a key is released.
		/// @param key	The code of the key that was released.
		virtual void OnKeyUp(int key);

		/// @brief		Handles the event when a key is held down and repeats.
		/// @param key	The code of the key that is being repeated.
		virtual void OnKeyRepeat(int key);

		/// @brief		Handles mouse movement events at the specified coordinates.
		/// @param x	The x-coordinate of the mouse pointer.
		/// @param y	The y-coordinate of the mouse pointer.
		virtual void OnMouseMove(double x, double y);

		/// @brief			Handles the event when a mouse button is pressed.
		/// @param button	The mouse button that was pressed.
		/// @param x		The x-coordinate of the mouse pointer.
		/// @param y		The y-coordinate of the mouse pointer.
		virtual void OnMouseButtonDown(Button button, double x, double y);

		/// @brief			Handles the event when a mouse button is released.
		/// @param button	The mouse button that was released.
		/// @param x		The x-coordinate of the mouse pointer.
		/// @param y		The y-coordinate of the mouse pointer.
		virtual void OnMouseButtonUp(Button button, double x, double y);

		/// @brief			called at each frame, you can update resource here
		/// @param id		identify in which thread this function is called
		/// @param renderer renderer interface
		virtual void OnUpdate(std::thread::id const& id, Renderer& renderer);

		/// @brief					called at each frame, commands should be recorded and then submitted to renderer.
		/// @param id				identify in which thread this function is called.
		/// @param command_object	command object interface
		virtual void OnRender(std::thread::id const& id, CommandObject& command_object);

	};

#ifndef STATIC_LIB
	/// @brief Call this to launch app with your derived instance;
	EXTERN_C DLL_API int DLL_CALL RunApp(IApplication* app) NO_EXCEPT;
#endif // !STATIC_LIB
	/// @brief Call this to stop the fyuu_engine instance.
	EXTERN_C DLL_API void DLL_CALL RequestStop() NO_EXCEPT;

} // namespace fyuu_engine::application


#endif // __cplusplus

#ifdef STATIC_LIB
extern int main(int argc, char** argv);
#endif

#endif // !FYUU_APPLICATION_H