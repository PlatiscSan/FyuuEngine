module fyuu_engine:interface_impl;
import :application;

namespace {
	using namespace fyuu_engine;
	application::Application* s_app;
} // namespace

namespace fyuu_engine {
	
	void InitializeApplication(int argc, char** argv) {
		static application::Application app(argc, argv);
		s_app = &app;
	}

	int Run() {
		s_app->Run();
	}

} // namespace fyuu_engine