
#include "pch.h"
#include "FyuuEntry.h"

int main(int argc, char** argv) {

	Fyuu::Application::ApplicationPtr app;

	try {

#if defined(_WIN32)
		app = Fyuu::InitWindowsApp(argc, argv);
#elif defined(__linux__)

#endif // defined(_WIN32)

		while (!app->IsQuit()) {
			app->Tick();
		}

	}
	catch (std::exception const&) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;

}