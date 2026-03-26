module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#endif
export module fyuu_engine:application;
#if defined(__cpp_lib_modules)
import std;
#endif
import :application_types;
import :rhi_context;

namespace fyuu_engine::application {

	export class Application {
	private:
		UniqueApplication m_impl;

	public:
		Application(int argc, char** argv);

		int Run() noexcept;

		float GetDisplayContentScale() const noexcept;

		rendering::RHIContext* GetRHIContext() noexcept;

		ApplicationBackend GetBackEnd() const noexcept;

		void* GetSurfaceHandle() noexcept;

	};

}
