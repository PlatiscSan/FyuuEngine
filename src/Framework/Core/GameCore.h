#ifndef GAME_CORE_H
#define GAME_CORE_H

#include "Window.h"

namespace Fyuu::core {

	void SetMainWindow(std::shared_ptr<Window> const& window);
	std::shared_ptr<Window> const& MainWindow();

	void SetUpdateCallback(std::function<void()> const& callback);
	void RequestQuit();
	bool UpdateApplication();

	void SetMessageLoop(std::function<void()> const& msg_loop);

	
	void InitializeWindowsApp(int argc, char** argv);
	void InitializeLinuxApp(int argc, char** argv);

}

#endif