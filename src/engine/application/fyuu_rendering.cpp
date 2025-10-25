#include <fyuu_rendering.h>

#ifdef WIN32
import d3d12_backend;
import windows_vulkan;
import windows_opengl;
#endif // WIN32
import static_hash_map;

#if defined(_WIN32) || defined(_WIN64)

#define CALL_COMMAND_OBJECT_MEMBER(BACKEND, FUNC, OBJ, ...) \
	FYUU_DECLARE_BACKEND_SWITCH( \
		BACKEND, \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::vulkan::VulkanCommandObject, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::opengl::OpenGLCommandObject, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::windows::d3d12::D3D12CommandObject, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		; \
	)\

#define BEGIN_RENDER_PASS(BACKEND, OBJ, OUTPUT_TARGET, ...) \
	FYUU_DECLARE_BACKEND_SWITCH( \
		BACKEND, \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::vulkan::VulkanCommandObject, BeginRenderPass, OBJ, *static_cast<fyuu_engine::vulkan::VulkanFrameBuffer*>(OUTPUT_TARGET), __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::opengl::OpenGLCommandObject, BeginRenderPass, OBJ, *static_cast<fyuu_engine::opengl::OpenGLFrameBuffer*>(OUTPUT_TARGET), __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::windows::d3d12::D3D12CommandObject, BeginRenderPass, OBJ, *static_cast<fyuu_engine::windows::d3d12::D3D12RenderTarget*>(OUTPUT_TARGET), __VA_OPT__(__VA_ARGS__)), \
		; \
	)\

#define END_RENDER_PASS(BACKEND, OBJ, OUTPUT_TARGET) \
	FYUU_DECLARE_BACKEND_SWITCH( \
		BACKEND, \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::vulkan::VulkanCommandObject, EndRenderPass, OBJ, *static_cast<fyuu_engine::vulkan::VulkanFrameBuffer*>(OUTPUT_TARGET)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::opengl::OpenGLCommandObject, EndRenderPass, OBJ, *static_cast<fyuu_engine::opengl::OpenGLFrameBuffer*>(OUTPUT_TARGET)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::windows::d3d12::D3D12CommandObject, EndRenderPass, OBJ, *static_cast<fyuu_engine::windows::d3d12::D3D12RenderTarget*>(OUTPUT_TARGET)), \
		; \
	)\

#elif defined(__linux__)

#define CALL_COMMAND_OBJECT_MEMBER(BACKEND, FUNC, OBJ, ...) \
	FYUU_DECLARE_BACKEND_SWITCH( \
		BACKEND, \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::vulkan::VulkanCommandObject, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::opengl::OpenGLCommandObject, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		; \
	)\

#define BEGIN_RENDER_PASS(BACKEND, OBJ, OUTPUT_TARGET, ...) \
	FYUU_DECLARE_BACKEND_SWITCH( \
		BACKEND, \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::vulkan::VulkanCommandObject, BeginRenderPass, OBJ, *static_cast<fyuu_engine::vulkan::VulkanFrameBuffer*>(OUTPUT_TARGET), __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::opengl::OpenGLCommandObject, BeginRenderPass, OBJ, *static_cast<fyuu_engine::opengl::OpenGLFrameBuffer*>(OUTPUT_TARGET), __VA_OPT__(__VA_ARGS__)), \
		; \
	)\

#define END_RENDER_PASS(BACKEND, OBJ, OUTPUT_TARGET) \
	FYUU_DECLARE_BACKEND_SWITCH( \
		BACKEND, \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::vulkan::VulkanCommandObject, EndRenderPass, OBJ, *static_cast<fyuu_engine::vulkan::VulkanFrameBuffer*>(OUTPUT_TARGET)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::opengl::OpenGLCommandObject, EndRenderPass, OBJ, *static_cast<fyuu_engine::opengl::OpenGLFrameBuffer*>(OUTPUT_TARGET)), \
		; \
	)\

#elif defined(__APPLE__) && defined(__MACH__)

#define CALL_COMMAND_OBJECT_MEMBER(BACKEND, FUNC, OBJ, ...) \
	FYUU_DECLARE_BACKEND_SWITCH( \
		BACKEND, \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::vulkan::VulkanCommandObject, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::opengl::OpenGLCommandObject, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::apple::metal::MetalCommandObject, FUNC, OBJ, __VA_OPT__(__VA_ARGS__)), \
		; \
	)\

#define BEGIN_RENDER_PASS(BACKEND, OBJ, OUTPUT_TARGET, ...) \
	FYUU_DECLARE_BACKEND_SWITCH( \
		BACKEND, \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::vulkan::VulkanCommandObject, BeginRenderPass, OBJ, *static_cast<fyuu_engine::vulkan::VulkanFrameBuffer*>(OUTPUT_TARGET), __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::opengl::OpenGLCommandObject, BeginRenderPass, OBJ, *static_cast<fyuu_engine::opengl::OpenGLFrameBuffer*>(OUTPUT_TARGET), __VA_OPT__(__VA_ARGS__)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::apple::metal::MetalCommandObject, BeginRenderPass, OBJ, *static_cast<fyuu_engine::apple::metal::MetalFrameBuffer*>(OUTPUT_TARGET), __VA_OPT__(__VA_ARGS__)), \
		; \
	)\

#define END_RENDER_PASS(BACKEND, OBJ, OUTPUT_TARGET) \
	FYUU_DECLARE_BACKEND_SWITCH( \
		BACKEND, \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::vulkan::VulkanCommandObject, EndRenderPass, OBJ, *static_cast<fyuu_engine::vulkan::VulkanFrameBuffer*>(OUTPUT_TARGET)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::opengl::OpenGLCommandObject, EndRenderPass, OBJ, *static_cast<fyuu_engine::opengl::OpenGLFrameBuffer*>(OUTPUT_TARGET)), \
		FYUU_CALL_BACKEND_OBJ_MEMBER(fyuu_engine::apple::metal::MetalCommandObject, EndRenderPass, OBJ, *static_cast<fyuu_engine::apple::metal::MetalFrameBuffer*>(OUTPUT_TARGET)), \
		; \
	)\


#endif



namespace fyuu_engine::application {

	CommandObject::CommandObject(Fyuu_CommandObject const& c_obj)
		: m_impl(c_obj.impl), m_output_target(c_obj.output_target), m_backend(static_cast<API>(c_obj.backend)) {

	}

	CommandObject::CommandObject(void* impl, void* output_target, API backend)
		: m_impl(impl), m_output_target(output_target), m_backend(backend) {

	}

	void CommandObject::BeginRecording() {
		CALL_COMMAND_OBJECT_MEMBER(m_backend, BeginRecording, m_impl);
	}

	void CommandObject::EndRecording() {
		CALL_COMMAND_OBJECT_MEMBER(m_backend, EndRecording, m_impl);
	}

	void CommandObject::Reset() {
		CALL_COMMAND_OBJECT_MEMBER(m_backend, Reset, m_impl);
	}

	void CommandObject::SetViewport(Viewport const& viewport) {
		CALL_COMMAND_OBJECT_MEMBER(m_backend, SetViewport, m_impl, reinterpret_cast<core::Viewport const&>(viewport));
	}

	void CommandObject::SetScissorRect(Rect const& rect) {
		CALL_COMMAND_OBJECT_MEMBER(m_backend, SetScissorRect, m_impl, reinterpret_cast<core::Rect const&>(rect));
	}

	CommandObject::operator Fyuu_CommandObject() const noexcept {
		return Fyuu_CommandObject{ m_impl, m_output_target, static_cast<Fyuu_API>(m_backend) };
	}

	void CommandObject::BeginRenderPass(std::span<float> clear_value) {
		BEGIN_RENDER_PASS(m_backend, m_impl, m_output_target, clear_value);
	}

	void CommandObject::BeginRenderPass(float r, float g, float b, float a) {
		BEGIN_RENDER_PASS(m_backend, m_impl, m_output_target, r, g, b, a);
	}

	void CommandObject::EndRenderPass() {
		END_RENDER_PASS(m_backend, m_impl, m_output_target);
	}

	Renderer::Renderer(Fyuu_Renderer const& c_obj) 
		: m_impl(c_obj.impl), m_backend(static_cast<API>(c_obj.backend)) {

	}

	Renderer::Renderer(void* impl, API backend) 
		: m_impl(impl), m_backend(backend) {

	}

	void* Renderer::GetImplementation() const noexcept {
		return m_impl;
	}

	API Renderer::GetBackend() const noexcept {
		return m_backend;
	}

	std::size_t Renderer::QueryObjectRequiredSize(QueryableObjectTypes type) const noexcept {
		
		static concurrency::StaticHashMap<QueryableObjectTypes, std::size_t, 64u> vulkan_sizes = {
			{QueryableObjectTypes::PSOBuilder, 64},
		};

		static concurrency::StaticHashMap<QueryableObjectTypes, std::size_t, 64u> opengl_sizes = {
			{QueryableObjectTypes::PSOBuilder, 64},
		};

#if defined(_WIN32) || defined(_WIN64)
		static concurrency::StaticHashMap<QueryableObjectTypes, std::size_t, 64u> direct12_sizes = {
			{QueryableObjectTypes::PSOBuilder, sizeof(windows::d3d12::D3D12PipelineStateObjectBuilder)},
		};

		FYUU_DECLARE_BACKEND_RETURN_SWITCH(m_backend, vulkan_sizes.UnsafeAt(type), opengl_sizes.UnsafeAt(type), direct12_sizes.UnsafeAt(type), 0);
#elif defined(__linux__)
		FYUU_DECLARE_BACKEND_RETURN_SWITCH(m_backend, vulkan_sizes.UnsafeAt(type), opengl_sizes.UnsafeAt(type), 0);
#elif defined(__APPLE__) && defined(__MACH__)

#endif

	}

	PSOBuilder Renderer::CreatePSOBuilder(std::span<std::byte> buffer) const noexcept {

		if (buffer.size() < QueryObjectRequiredSize(QueryableObjectTypes::PSOBuilder)) {
			throw std::invalid_argument("Insufficient buffer");
		}



	}

	Renderer::operator Fyuu_Renderer() const noexcept {
		return Fyuu_Renderer{ m_impl, static_cast<Fyuu_API>(m_backend) };
	}

}

DLL_API void DLL_CALL Fyuu_BeginRecording(Fyuu_CommandObject* command_object) {
	CALL_COMMAND_OBJECT_MEMBER(command_object->backend, BeginRecording, command_object->impl);
}

DLL_API void DLL_CALL Fyuu_EndRecording(Fyuu_CommandObject* command_object) {
	CALL_COMMAND_OBJECT_MEMBER(command_object->backend, EndRecording, command_object->impl);
}

DLL_API void DLL_CALL Fyuu_ResetCommandObject(Fyuu_CommandObject* command_object) {
	CALL_COMMAND_OBJECT_MEMBER(command_object->backend, Reset, command_object->impl);
}

DLL_API void DLL_CALL Fyuu_SetViewport(Fyuu_CommandObject* command_object, Fyuu_Viewport const* viewport) {
	CALL_COMMAND_OBJECT_MEMBER(command_object->backend, SetViewport, command_object->impl, reinterpret_cast<fyuu_engine::core::Viewport const&>(*viewport));
}

DLL_API void DLL_CALL Fyuu_SetScissorRect(Fyuu_CommandObject* command_object, Fyuu_Rect const* rect) {
	CALL_COMMAND_OBJECT_MEMBER(command_object->backend, SetScissorRect, command_object->impl, reinterpret_cast<fyuu_engine::core::Rect const&>(*rect));
}

DLL_API void DLL_CALL Fyuu_BeginRenderPass(Fyuu_CommandObject* command_object, float r, float g, float b, float a) {
	BEGIN_RENDER_PASS(command_object->backend, command_object->impl, command_object->output_target, r, g, b, a);
}

DLL_API void DLL_CALL Fyuu_EndRenderPass(Fyuu_CommandObject* command_object) {
	END_RENDER_PASS(command_object->backend, command_object->impl, command_object->output_target);
}