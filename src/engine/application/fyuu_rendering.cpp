#include <fyuu_rendering.h>

#ifdef WIN32
import d3d12_backend;
import windows_vulkan;
import windows_opengl;
#endif // WIN32

namespace fyuu_engine::application {

	/*
	*	helper macros to call implementation object
	*/

#ifdef _WIN32
	#define CALL_BACKEND_FUNC(FUNC_NAME, BACKEND, IMPL, ...) \
	switch (static_cast<API>(BACKEND)) { \
		case API::Vulkan: \
			static_cast<vulkan::VulkanCommandObject*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
			break;\
		case API::OpenGL:\
			static_cast<opengl::OpenGLCommandObject*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
			break;\
		case API::DirectX12: \
			static_cast<windows::d3d12::D3D12CommandObject*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
			break;\
		default: \
			break; \
	}
#else
	#define CALL_BACKEND_FUNC(FUNC_NAME, BACKEND, IMPL, ...) \
	switch (static_cast<API>(BACKEND)) { \
		case API::Vulkan: \
			static_cast<vulkan::VulkanCommandObject*>(IMPL)->FUNC_NAME(__VA_OPT__(,) __VA_ARGS__); \
			break;\
		case API::OpenGL:\
			static_cast<opengl::OpenGLCommandObject*>(IMPL)->FUNC_NAME(__VA_OPT__(,) __VA_ARGS__); \
			break;\
		default: \
			break; \
}
#endif // _WIN32

	CommandObject::CommandObject(Fyuu_CommandObject const& c_obj)
		: m_impl(c_obj.impl), m_output_target(c_obj.output_target), m_backend(static_cast<API>(c_obj.backend)) {

	}

	CommandObject::CommandObject(void* impl, void* output_target, API backend)
		: m_impl(impl), m_output_target(output_target), m_backend(backend) {

	}

	void CommandObject::BeginRecording() {
		CALL_BACKEND_FUNC(BeginRecording, m_backend, m_impl);
	}

	void CommandObject::EndRecording() {
		CALL_BACKEND_FUNC(EndRecording, m_backend, m_impl);
	}

	void CommandObject::Reset() {
		CALL_BACKEND_FUNC(Reset, m_backend, m_impl);
	}

	void CommandObject::SetViewport(Viewport const& viewport) {
		CALL_BACKEND_FUNC(SetViewport, m_backend, m_impl, reinterpret_cast<core::Viewport const&>(viewport));
	}

	void CommandObject::SetScissorRect(Rect const& rect) {
		CALL_BACKEND_FUNC(SetScissorRect, m_backend, m_impl, reinterpret_cast<core::Rect const&>(rect));
	}

	CommandObject::operator Fyuu_CommandObject() const noexcept {
		return Fyuu_CommandObject{ m_impl, m_output_target, static_cast<Fyuu_API>(m_backend) };
	}

	void CommandObject::BeginRenderPass(std::span<float> clear_value) {
		switch (m_backend) {
		case API::Vulkan:
			static_cast<fyuu_engine::vulkan::VulkanCommandObject*>(m_impl)->BeginRenderPass(*static_cast<fyuu_engine::vulkan::VulkanFrameBuffer*>(m_output_target), clear_value);
			break;
		case API::OpenGL:
			static_cast<fyuu_engine::opengl::OpenGLCommandObject*>(m_impl)->BeginRenderPass(*static_cast<fyuu_engine::opengl::OpenGLFrameBuffer*>(m_output_target), clear_value);
			break;
#ifdef _WIN32
		case API::DirectX12:
			static_cast<fyuu_engine::windows::d3d12::D3D12CommandObject*>(m_impl)->BeginRenderPass(*static_cast<fyuu_engine::windows::d3d12::D3D12RenderTarget*>(m_output_target), clear_value);
			break;
#endif // _WIN32
		default:
			break;
		};
	}

	void CommandObject::BeginRenderPass(float r, float g, float b, float a) {
		switch (m_backend) {
		case API::Vulkan:
			static_cast<fyuu_engine::vulkan::VulkanCommandObject*>(m_impl)->BeginRenderPass(*static_cast<fyuu_engine::vulkan::VulkanFrameBuffer*>(m_output_target), r, g, b, a);
			break;
		case API::OpenGL:
			static_cast<fyuu_engine::opengl::OpenGLCommandObject*>(m_impl)->BeginRenderPass(*static_cast<fyuu_engine::opengl::OpenGLFrameBuffer*>(m_output_target), r, g, b, a);
			break;
#ifdef _WIN32
		case API::DirectX12:
			static_cast<fyuu_engine::windows::d3d12::D3D12CommandObject*>(m_impl)->BeginRenderPass(*static_cast<fyuu_engine::windows::d3d12::D3D12RenderTarget*>(m_output_target), r, g, b, a);
			break;
#endif // _WIN32
		default:
			break;
		};
	}

	void CommandObject::EndRenderPass() {
		switch (m_backend) {
		case API::Vulkan:
			static_cast<fyuu_engine::vulkan::VulkanCommandObject*>(m_impl)->EndRenderPass(*static_cast<fyuu_engine::vulkan::VulkanFrameBuffer*>(m_output_target));
			break;
		case API::OpenGL:
			static_cast<fyuu_engine::opengl::OpenGLCommandObject*>(m_impl)->EndRenderPass(*static_cast<fyuu_engine::opengl::OpenGLFrameBuffer*>(m_output_target));
			break;
#ifdef _WIN32
		case API::DirectX12:
			static_cast<fyuu_engine::windows::d3d12::D3D12CommandObject*>(m_impl)->EndRenderPass(*static_cast<fyuu_engine::windows::d3d12::D3D12RenderTarget*>(m_output_target));
			break;
#endif // _WIN32
		default:
			break;
		};
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

	Renderer::operator Fyuu_Renderer() const noexcept {
		return Fyuu_Renderer{ m_impl, static_cast<Fyuu_API>(m_backend) };
	}

}

#ifdef _WIN32
#define C_INTERFACE_CALL_BACKEND_FUNC(FUNC_NAME, BACKEND, IMPL, ...) \
switch (static_cast<Fyuu_API>(BACKEND)) { \
	case Fyuu_API::FYUU_API_VULKAN: \
		static_cast<fyuu_engine::vulkan::VulkanCommandObject*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
		break;\
	case Fyuu_API::FYUU_API_OPENGL:\
		static_cast<fyuu_engine::opengl::OpenGLCommandObject*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
		break;\
	case Fyuu_API::FYUU_API_DIRECTX12: \
		static_cast<fyuu_engine::windows::d3d12::D3D12CommandObject*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
		break;\
	default: \
		break; \
}
#else
#define CALL_BACKEND_FUNC(FUNC_NAME, BACKEND, IMPL, ...) \
switch (static_cast<API>(BACKEND)) { \
	case API::Vulkan: \
		static_cast<fyuu_engine::vulkan::VulkanCommandObject*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
		break;\
	case API::OpenGL:\
		static_cast<fyuu_engine::opengl::OpenGLCommandObject*>(IMPL)->FUNC_NAME(__VA_OPT__(__VA_ARGS__)); \
		break;\
	default: \
		break; \
}
#endif // _WIN32

DLL_API void DLL_CALL Fyuu_BeginRecording(Fyuu_CommandObject* command_object) {
	C_INTERFACE_CALL_BACKEND_FUNC(BeginRecording, command_object->backend, command_object->impl);
}

DLL_API void DLL_CALL Fyuu_EndRecording(Fyuu_CommandObject* command_object) {
	C_INTERFACE_CALL_BACKEND_FUNC(EndRecording, command_object->backend, command_object->impl);
}

DLL_API void DLL_CALL Fyuu_ResetCommandObject(Fyuu_CommandObject* command_object) {
	C_INTERFACE_CALL_BACKEND_FUNC(Reset, command_object->backend, command_object->impl);
}

DLL_API void DLL_CALL Fyuu_SetViewport(Fyuu_CommandObject* command_object, Fyuu_Viewport const* viewport) {
	C_INTERFACE_CALL_BACKEND_FUNC(SetViewport, command_object->backend, command_object->impl, reinterpret_cast<fyuu_engine::core::Viewport const&>(*viewport));
}

DLL_API void DLL_CALL Fyuu_SetScissorRect(Fyuu_CommandObject* command_object, Fyuu_Rect const* rect) {
	C_INTERFACE_CALL_BACKEND_FUNC(SetScissorRect, command_object->backend, command_object->impl, reinterpret_cast<fyuu_engine::core::Rect const&>(*rect));
}

DLL_API void DLL_CALL Fyuu_BeginRenderPass(Fyuu_CommandObject* command_object, float r, float g, float b, float a) {
	switch (command_object->backend) {
	case Fyuu_API::FYUU_API_VULKAN: 
		static_cast<fyuu_engine::vulkan::VulkanCommandObject*>(command_object->impl)->BeginRenderPass(*static_cast<fyuu_engine::vulkan::VulkanFrameBuffer*>(command_object->output_target), r, g, b, a);
		break; 
	case Fyuu_API::FYUU_API_OPENGL: 
		static_cast<fyuu_engine::opengl::OpenGLCommandObject*>(command_object->impl)->BeginRenderPass(*static_cast<fyuu_engine::opengl::OpenGLFrameBuffer*>(command_object->output_target), r, g, b, a);
		break; 
#ifdef _WIN32
	case Fyuu_API::FYUU_API_DIRECTX12: 
		static_cast<fyuu_engine::windows::d3d12::D3D12CommandObject*>(command_object->impl)->BeginRenderPass(*static_cast<fyuu_engine::windows::d3d12::D3D12RenderTarget*>(command_object->output_target), r, g, b, a);
		break; 
#endif // _WIN32
	default: 
		break;
	};
}

DLL_API void DLL_CALL Fyuu_EndRenderPass(Fyuu_CommandObject* command_object) {
	switch (command_object->backend) {
	case Fyuu_API::FYUU_API_VULKAN:
		static_cast<fyuu_engine::vulkan::VulkanCommandObject*>(command_object->impl)->EndRenderPass(*static_cast<fyuu_engine::vulkan::VulkanFrameBuffer*>(command_object->output_target));
		break;
	case Fyuu_API::FYUU_API_OPENGL:
		static_cast<fyuu_engine::opengl::OpenGLCommandObject*>(command_object->impl)->EndRenderPass(*static_cast<fyuu_engine::opengl::OpenGLFrameBuffer*>(command_object->output_target));
		break;
#ifdef _WIN32
	case Fyuu_API::FYUU_API_DIRECTX12:
		static_cast<fyuu_engine::windows::d3d12::D3D12CommandObject*>(command_object->impl)->EndRenderPass(*static_cast<fyuu_engine::windows::d3d12::D3D12RenderTarget*>(command_object->output_target));
		break;
#endif // _WIN32
	default:
		break;
	};
}