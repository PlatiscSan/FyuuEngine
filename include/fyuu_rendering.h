#ifndef FYUU_RENDERING_H
#define FYUU_RENDERING_H

#include <fyuu_logger.h>
#ifdef __cplusplus
#include <array>
#include <span>
#else
#include <stdalign.h>
#endif // __cplusplus


EXTERN_C {

	typedef struct Fyuu_Viewport {
		float    x;
		float    y;
		float    width;
		float    height;
		float    min_depth;
		float    max_depth;
	} Fyuu_Viewport;

	typedef struct Fyuu_Rect {
		int32_t x;
		int32_t y;
		uint32_t width;
		uint32_t height;
	} Fyuu_Rect;

	typedef enum Fyuu_API {
		FYUU_API_UNKNOWN,
		FYUU_API_VULKAN,
		FYUU_API_DIRECTX12,
		FYUU_API_METAL,
		FYUU_API_OPENGL,
	} Fyuu_API;

	typedef struct Fyuu_CommandObject {
		void* impl;
		void* output_target;
		Fyuu_API backend;
	} Fyuu_CommandObject;

	typedef struct Fyuu_Renderer {
		void* impl;
		Fyuu_API backend;
	} Fyuu_Renderer;
	
	DLL_API void DLL_CALL Fyuu_BeginRecording(Fyuu_CommandObject* command_object);
	DLL_API void DLL_CALL Fyuu_EndRecording(Fyuu_CommandObject* command_object);
	DLL_API void DLL_CALL Fyuu_ResetCommandObject(Fyuu_CommandObject* command_object);
	DLL_API void DLL_CALL Fyuu_SetViewport(Fyuu_CommandObject* command_object, Fyuu_Viewport const* viewport);
	DLL_API void DLL_CALL Fyuu_SetScissorRect(Fyuu_CommandObject* command_object, Fyuu_Rect const* rect);
	DLL_API void DLL_CALL Fyuu_BeginRenderPass(Fyuu_CommandObject* command_object, float r, float g, float b, float a);
	DLL_API void DLL_CALL Fyuu_EndRenderPass(Fyuu_CommandObject* command_object);
}

#ifdef __cplusplus
namespace fyuu_engine::application {

	struct Viewport : public Fyuu_Viewport {};

	struct Rect : public Fyuu_Rect {};

	enum class API : std::uint8_t {
		Unknown,
		Vulkan,	
		DirectX12,
		Metal,
		OpenGL,
	};

	enum class QueryableObjectTypes : std::uint8_t {
		PSOBuilder
	};

	class DLL_API CommandObject {
	private:
		void* m_impl;
		void* m_output_target;
		API m_backend;

	public:
		CommandObject(Fyuu_CommandObject const& c_obj);
		CommandObject(void* impl, void* output_target, API backend);

		void BeginRecording();
		void EndRecording();
		void Reset();
		void SetViewport(Viewport const& viewport);
		void SetScissorRect(Rect const& rect);
		void BeginRenderPass(std::span<float> clear_value);
		void BeginRenderPass(float r, float g, float b, float a);
		void EndRenderPass();

		operator Fyuu_CommandObject() const noexcept;

	};

	class DLL_API PSOBuilder {
	private:
		void* m_impl;
		API m_backend;

	public:

	};

	class DLL_API Renderer {
	private:
		void* m_impl;
		API m_backend;

	public:
		Renderer(Fyuu_Renderer const& c_obj);
		Renderer(void* impl, API backend);

		void* GetImplementation() const noexcept;
		API GetBackend() const noexcept;

		std::size_t QueryObjectRequiredSize(QueryableObjectTypes type) const noexcept;

		PSOBuilder CreatePSOBuilder(std::span<std::byte> buffer) const noexcept;

		operator Fyuu_Renderer() const noexcept;
	};
}

/*
*	helper macros to call internal implementation object
*/

#define FYUU_CALL_BACKEND_OBJ_MEMBER(OBJ_TYPE, FUNC, OBJ, ...) \
	static_cast<OBJ_TYPE*>(OBJ)->FUNC(__VA_OPT__(__VA_ARGS__))

#if defined(_WIN32) || defined(_WIN64)

#define FYUU_DECLARE_BACKEND_SWITCH(BACKEND, VULKAN_CASE, OPENGL_CASE, DIRECTX_12_CASE, DEFAULT_CASE) \
	switch (static_cast<fyuu_engine::application::API>(BACKEND)) { \
		case fyuu_engine::application::API::Vulkan: \
			VULKAN_CASE; \
			break;\
		case fyuu_engine::application::API::DirectX12: \
			DIRECTX_12_CASE; \
			break;\
		case fyuu_engine::application::API::OpenGL:\
			OPENGL_CASE; \
			break;\
		default: \
			DEFAULT_CASE; \
			break; \
	}

#define FYUU_DECLARE_BACKEND_RETURN_SWITCH(BACKEND, VULKAN_CASE, OPENGL_CASE, DIRECTX_12_CASE, DEFAULT_CASE) \
	switch (static_cast<fyuu_engine::application::API>(BACKEND)) { \
		case fyuu_engine::application::API::Vulkan: \
			return VULKAN_CASE; \
		case fyuu_engine::application::API::DirectX12: \
			return DIRECTX_12_CASE; \
		case fyuu_engine::application::API::OpenGL:\
			return OPENGL_CASE; \
		default: \
			return DEFAULT_CASE; \
	}

#elif defined(__linux__)

#define FYUU_DECLARE_BACKEND_SWITCH(BACKEND, VULKAN_CASE, OPENGL_CASE, DEFAULT_CASE) \
	switch (static_cast<fyuu_engine::application::API>(BACKEND)) { \
		case fyuu_engine::application::API::Vulkan: \
			VULKAN_CASE; \
			break;\
		case fyuu_engine::application::API::OpenGL:\
			OPENGL_CASE; \
			break;\
		default: \
			DEFAULT_CASE; \
			break; \
	}

#define FYUU_DECLARE_BACKEND_RETURN_SWITCH(BACKEND, VULKAN_CASE, OPENGL_CASE, DEFAULT_CASE) \
	switch (static_cast<fyuu_engine::application::API>(BACKEND)) { \
		case fyuu_engine::application::API::Vulkan: \
			return VULKAN_CASE; \
		case fyuu_engine::application::API::OpenGL:\
			return OPENGL_CASE; \
		default: \
			return DEFAULT_CASE; \
	}

#elif defined(__APPLE__) && defined(__MACH__)

#define FYUU_DECLARE_BACKEND_SWITCH(BACKEND, VULKAN_CASE, OPENGL_CASE, METAL_CASE, DEFAULT_CASE) \
	switch (static_cast<fyuu_engine::application::API>(BACKEND)) { \
		case fyuu_engine::application::API::Vulkan: \
			VULKAN_CASE; \
			break;\
		case fyuu_engine::application::API::Metal: \
			METAL_CASE; \
			break;\
		case fyuu_engine::application::API::OpenGL:\
			OPENGL_CASE; \
			break;\
		default: \
			DEFAULT_CASE; \
			break; \
	}

#define FYUU_DECLARE_BACKEND_SWITCH(BACKEND, VULKAN_CASE, OPENGL_CASE, METAL_CASE, DEFAULT_CASE) \
	switch (static_cast<fyuu_engine::application::API>(BACKEND)) { \
		case fyuu_engine::application::API::Vulkan: \
			return VULKAN_CASE; \
		case fyuu_engine::application::API::Metal: \
			return METAL_CASE; \
		case fyuu_engine::application::API::OpenGL:\
			return OPENGL_CASE; \
		default: \
			return DEFAULT_CASE; \
	}

#endif

#endif // __cplusplus

#endif // !FYUU_RENDERING_H
