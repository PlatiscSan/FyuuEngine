#pragma once

#include "export_macros.h"
#include "fyuu_graphics.h"

#if defined(BUILD_STATIC_LIBS)

typedef struct FyuuApplication {
	int (*Init)(struct FyuuApplication* self, int argc, char** argv);
	void (*Tick)(struct FyuuApplication* self);
	bool (*ShouldQuit)(struct FyuuApplication* self);
	void (*CleanUp)(struct FyuuApplication* self);

	void* user_data;
} FyuuApplication;

EXTERN_C FyuuApplication* FyuuCreateApplication();

#if defined(__cplusplus)
namespace fyuu_engine {

	class Application
		: private FyuuApplication {

	public:	
		using Base = FyuuApplication;

		/// @brief call this in FyuuCreateApplication() then return
		/// @param app 
		/// @return 
		static FyuuApplication* AsCInterface(Application* app);

		Application();

		virtual ~Application() = default;
		virtual int Init(int argc, char** argv) = 0;
		virtual void Tick() = 0;
		virtual bool ShouldQuit() const = 0;
		virtual void CleanUp() = 0;
	};



}
#endif // defined(__cplusplus)

#endif // defined(BUILD_STATIC_LIBS)