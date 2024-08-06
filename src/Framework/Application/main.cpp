
#include "pch.h"
#include "FyuuEntry.h"

int main(int argc, char** argv) {

	std::shared_ptr<Fyuu::Application> app;

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