
#include "fyuu_application.h"

import std;

#if defined(BUILD_STATIC_LIBS)

static int CApp_Init(FyuuApplication* self, int argc, char** argv) {
	if (self->user_data) {
		fyuu_engine::Application* app =
			static_cast<fyuu_engine::Application*>(self->user_data);
		return app->Init(argc, argv);
	}
	return -1;
}

static void CApp_Tick(FyuuApplication* self) {
	if (self->user_data) {
		fyuu_engine::Application* app =
			static_cast<fyuu_engine::Application*>(self->user_data);
		app->Tick();
	}
}

static bool CApp_ShouldQuit(FyuuApplication* self) {
	if (self->user_data) {
		fyuu_engine::Application* app =
			static_cast<fyuu_engine::Application*>(self->user_data);
		return app->ShouldQuit();
	}
	return true;
}

static void CApp_CleanUp(FyuuApplication* self) {
	if (self->user_data) {
		fyuu_engine::Application* app =
			static_cast<fyuu_engine::Application*>(self->user_data);
		app->CleanUp();
	}
}

FyuuApplication* fyuu_engine::Application::AsCInterface(Application* app) {
	return app;
}

fyuu_engine::Application::Application() {
	Base::Init = CApp_Init;
	Base::Tick = CApp_Tick;
	Base::ShouldQuit = CApp_ShouldQuit;
	Base::CleanUp = CApp_CleanUp;
	Base::user_data = this;
}

int main(int argc, char** argv) try {

	FyuuApplication* app = FyuuCreateApplication();
	int init_result = app->Init(app, argc, argv);
	if (init_result != 0) {
		return init_result;
	}

	while (!app->ShouldQuit(app)) {
		app->Tick(app);
	}

	app->CleanUp(app);

	return 0;

}
catch (std::exception const& ex) {
	std::println("Unhandled fatal error: {}", ex.what());
	return -1;
}

#endif // defined(BUILD_STATIC_LIBS)