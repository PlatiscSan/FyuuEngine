#include <version>
#if defined(__cpp_lib_modules)
import std;
#else
#include <print>
#endif // defined(__cpp_lib_modules)
import fyuu_engine;

int main(int argc, char** argv) {
	try {
		fyuu_engine::InitializeApplication(argc, argv);
		return fyuu_engine::Run();
	}
	catch (fyuu_engine::application::ShowHelp const&) {
		return 0;
	}
	catch (std::exception const& ex) {
		std::println("{}", ex.what());
		return -1;
	}
	catch(...) {
		std::println("Unknown exception");
		return -1;
	}
}