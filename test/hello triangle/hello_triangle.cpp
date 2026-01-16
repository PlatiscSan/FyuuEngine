#include "fyuu_application.h"
#include "fyuu_event_bus.h"
#include "fyuu_graphics.h"

import std;

struct Vertex {
	std::array<float, 2> pos;
	std::array<float, 3> color;
};

class HelloTriangle : public fyuu_engine::Application {
private:
	fyuu_engine::PhysicalDevice m_physical_device;
	fyuu_engine::LogicalDevice m_logical_device;
	fyuu_engine::CommandQueue m_command_queue;
	fyuu_engine::Surface m_surface;
	fyuu_engine::SwapChain m_swap_chain;
	fyuu_engine::ShaderLibrary m_vertex_shader;
	fyuu_engine::VideoMemory m_vertex_buffer_mem;
	fyuu_engine::Resource m_vertex_buffer;
	fyuu_engine::ShaderLibrary m_fragment_shader;

	static char const* AppName() noexcept {
		return "Hello Triangle";
	}

	static inline fyuu_engine::PhysicalDevice CreatePhysicalDevice() {

		fyuu_engine::InitOptions init_option;

		init_option.app_name = AppName();
		init_option.app_version.variant = 0;
		init_option.app_version.major = 1;
		init_option.app_version.minor = 0;
		init_option.app_version.patch = 0;

		return fyuu_engine::PhysicalDevice(init_option, fyuu_engine::API::Vulkan);

	}

	static inline fyuu_engine::ShaderLibrary CreateVertexShader(fyuu_engine::LogicalDevice& logical_device) {

		std::string_view vert_src = R"(
#version 450

layout(push_constant) uniform UniformBufferObject {
    float time;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    float anim = ubo.time / 100.0;
    
    mat2 rotmat = mat2(
        vec2(cos(anim), -sin(anim)),
        vec2(sin(anim), cos(anim))
    );

    gl_Position = vec4(rotmat * inPosition, 0.5, 1.0);
    fragColor = inColor;
})";

		return fyuu_engine::ShaderLibrary(
			logical_device,
			vert_src,
			fyuu_engine::ShaderStage::Vertex,
			fyuu_engine::ShaderLanguage::GLSL
		);

	}

	static inline fyuu_engine::ShaderLibrary CreateFragmentShader(fyuu_engine::LogicalDevice& logical_device) {

		std::string_view frag_src = R"(
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;
void main() {
	outColor = vec4(fragColor, 1.0);
})";

		return fyuu_engine::ShaderLibrary(
			logical_device,
			frag_src,
			fyuu_engine::ShaderStage::Fragment,
			fyuu_engine::ShaderLanguage::GLSL
		);

	}

	static fyuu_engine::Resource CreateVertexBuffer(
		fyuu_engine::VideoMemory& video_memory,
		fyuu_engine::LogicalDevice& logical_device,
		fyuu_engine::CommandQueue& copy_queue
	) {

		constexpr static Vertex vertices[] = {
			{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
			{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
		};

		fyuu_engine::Resource vertex_buffer(video_memory, sizeof(vertices), 1u, 1u, fyuu_engine::ResourceType::VertexBuffer);
		vertex_buffer.SetBufferData(logical_device, copy_queue, { reinterpret_cast<std::byte const*>(&vertices), sizeof(vertices) });
	
		return vertex_buffer;

	}

public:
	HelloTriangle()
		: Application(),
		m_physical_device(HelloTriangle::CreatePhysicalDevice()),
		m_logical_device(m_physical_device),
		m_command_queue(m_logical_device, fyuu_engine::CommandObjectType::AllCommands),
		m_surface(m_physical_device, 1280, 720, fyuu_engine::SurfaceFlag::None),
		m_swap_chain(m_physical_device, m_logical_device, m_command_queue, m_surface),
		m_vertex_shader(HelloTriangle::CreateVertexShader(m_logical_device)),
		m_vertex_buffer_mem(m_logical_device, sizeof(Vertex) * 3ull, fyuu_engine::VideoMemoryUsage::VertexBuffer, fyuu_engine::VideoMemoryType::DeviceLocal),
		m_vertex_buffer(CreateVertexBuffer(m_vertex_buffer_mem, m_logical_device, m_command_queue)),
		m_fragment_shader(HelloTriangle::CreateFragmentShader(m_logical_device)) {

	}

	int Init(int argc, char** argv) override {

		return 0;
	}

	void Tick() override {

	}

	bool ShouldQuit() const override {
		return true;
	}

	void CleanUp() override {

	}
};

EXTERN_C FyuuApplication* FyuuCreateApplication() {
	static HelloTriangle app;
	return HelloTriangle::AsCInterface(&app);
}