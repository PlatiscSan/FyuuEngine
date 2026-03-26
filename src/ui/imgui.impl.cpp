module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <cstdint>
#include <memory>
#include <functional>
#include <atomic>
#include <future>
#include <unordered_map>
#include <variant>
#include <optional>
#endif // !defined(__cpp_lib_modules)
#include <tbb/tbb.h>
#include "fyuu_platform.hpp"
#include "fyuu_declare_pool.hpp"

/* imgui headers */

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_glfw.h>

module fyuu_engine:imgui_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :imgui;
import plastic.sealed_polymorphism;
import fyuu_rhi;
import :application;
import :ui;

namespace fyuu_engine::ui::imgui {
/* 
	void ImGUI::CompileShader() {

		auto& attachment = m_context.attachment;
		fyuu_rhi::ShaderDataSegment& segment = attachment->shader_data_segment.emplace(*m_context.logical_device);

		auto& platform_handle = m_context.surface->GetPlatformHandle();

		static constexpr char const* vulkan_vertex_shader_src = R"(
			#version 460 core
			layout(location = 0) in vec2 aPos;
			layout(location = 1) in vec2 aUV;
			layout(location = 2) in vec4 aColor;
			layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

			out gl_PerVertex { vec4 gl_Position; };
			layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

			void main() {
				Out.Color = aColor;
				Out.UV = aUV;
				gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
			}
			)";
		static constexpr char const* vulkan_fragment_shader_src = R"(
			#version 460 core
			layout(location = 0) out vec4 fColor;
			layout(set=0, binding=0) uniform texture2D sTexture;
			layout(set=0, binding=1) uniform sampler sSampler;
			layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
			void main() {
				fColor = In.Color * texture(sampler2D(sTexture, sSampler), In.UV);
			}
			)";

		static constexpr char const* d3d12_vertex_shader_src = R"(
			cbuffer vertexBuffer : register(b0) {
				float4x4 ProjectionMatrix; 
			};
			struct VS_INPUT {
				float2 pos : POSITION;
				float4 col : COLOR0;
				float2 uv  : TEXCOORD0;
			};
			struct PS_INPUT {
				float4 pos : SV_POSITION;
				float4 col : COLOR0;
				float2 uv  : TEXCOORD0;
			};
			PS_INPUT main(VS_INPUT input) {
				PS_INPUT output;
				output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
				output.col = input.col;
				output.uv  = input.uv;
				return output;
			}
			)";
		static constexpr char const* pixel_shader_src = R"(
			struct PS_INPUT {
				float4 pos : SV_POSITION;
				float4 col : COLOR0;
				float2 uv  : TEXCOORD0;
			};

			SamplerState sampler0 : register(s0);
			Texture2D texture0 : register(t0);
				
			float4 main(PS_INPUT input) : SV_Target {
				float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
				return out_col; 
			}			
			)";

		static constexpr char const* vertex_shader_es_src = R"(
			#version 310 es
			precision highp float;
			layout(location = 0) in vec2 Position;
			layout(location = 1) in vec2 UV;
			layout(location = 2) in vec4 Color;
			uniform mat4 ProjMtx;
			out vec2 Frag_UV;
			out vec4 Frag_Color;
			void main() {
				Frag_UV = UV;
				Frag_Color = Color;
				gl_Position = ProjMtx * vec4(Position.xy,0,1);
			}
			)";
		static constexpr char const* fragment_shader_es_src = R"(
			#version 310 es
			precision mediump float;
			uniform texture2D Texture;
			uniform sampler TextureSampler;
			in vec2 Frag_UV;
			in vec4 Frag_Color;
			layout (location = 0) out vec4 Out_Color;
			void main() {
				Out_Color = Frag_Color * texture(sampler2D(Texture, TextureSampler), Frag_UV.st);
			}
			)";
#if !defined(__ANDROID__)
		static constexpr char const* opengl_vertex_shader_src = R"(
			#version 460 core
			layout(location = 0) in vec2 Position;
			layout(location = 1) in vec2 UV;
			layout(location = 2) in vec4 Color;
			layout(binding = 0) uniform mat4 ProjMtx;
			out vec2 Frag_UV;
			out vec4 Frag_Color;
			void main() {
				Frag_UV = UV;
				Frag_Color = Color;
				gl_Position = ProjMtx * vec4(Position.xy,0,1);
			}
			)";
		static constexpr char const* opengl_fragment_shader_src = R"(
			#version 460 core
			precision mediump float;
			layout(binding = 0) uniform texture2D Texture;
			layout(binding = 1) uniform sampler TextureSampler;
			in vec2 Frag_UV;
			in vec4 Frag_Color;
			layout (location = 0) out vec4 Out_Color;
			void main() {
				Out_Color = Frag_Color * texture(sampler2D(Texture, TextureSampler), Frag_UV.st);
			}
			)";
#endif // !defined(__ANDROID__)
		switch (m_context.logical_device->GetAPI()) {
		case fyuu_rhi::API::Vulkan:
			attachment->vertex_shader.emplace(*m_context.logical_device, vulkan_vertex_shader_src, fyuu_rhi::ShaderStage::Vertex, fyuu_rhi::ShaderLanguage::GLSL, fyuu_rhi::ShaderCompilationOption{});
			attachment->fragment_shader.emplace(*m_context.logical_device, vulkan_fragment_shader_src, fyuu_rhi::ShaderStage::Fragment, fyuu_rhi::ShaderLanguage::GLSL, fyuu_rhi::ShaderCompilationOption{});
			segment.Declare(0u, 4u, fyuu_rhi::ShaderStage::Vertex)
				.Declare(fyuu_rhi::ResourceFlags::SampledTexture, 0u, 1u, fyuu_rhi::ShaderStage::Fragment)
				.Declare(
					fyuu_rhi::SamplerFlags::MagLinear | fyuu_rhi::SamplerFlags::MinLinear | fyuu_rhi::SamplerFlags::MipLinear | fyuu_rhi::SamplerFlags::ClampToEdge,
					{},
					1u,
					fyuu_rhi::ShaderStage::Fragment
				)
				.Instantiate();
			break;

		case fyuu_rhi::API::DirectX12:
			attachment->vertex_shader.emplace(*m_context.logical_device, d3d12_vertex_shader_src, fyuu_rhi::ShaderStage::Vertex, fyuu_rhi::ShaderLanguage::HLSL, fyuu_rhi::ShaderCompilationOption{});
			attachment->fragment_shader.emplace(*m_context.logical_device, pixel_shader_src, fyuu_rhi::ShaderStage::Pixel, fyuu_rhi::ShaderLanguage::HLSL, fyuu_rhi::ShaderCompilationOption{});
			segment.Declare(0u, 16u, fyuu_rhi::ShaderStage::Vertex)
				.Declare(fyuu_rhi::ResourceFlags::SampledTexture, 0u, 1u, fyuu_rhi::ShaderStage::Pixel)
				.Declare(
					fyuu_rhi::SamplerFlags::MagLinear | fyuu_rhi::SamplerFlags::MinLinear | fyuu_rhi::SamplerFlags::MipLinear | fyuu_rhi::SamplerFlags::ClampToEdge,
					{},
					0u,
					fyuu_rhi::ShaderStage::Pixel
				)
				.Instantiate();
			break;

		case fyuu_rhi::API::OpenGL:
#if defined(_WIN32)
			attachment->vertex_shader.emplace(*m_context.logical_device, opengl_vertex_shader_src, fyuu_rhi::ShaderStage::Vertex, fyuu_rhi::ShaderLanguage::GLSL, fyuu_rhi::ShaderCompilationOption{});
			attachment->fragment_shader.emplace(*m_context.logical_device, opengl_fragment_shader_src, fyuu_rhi::ShaderStage::Fragment, fyuu_rhi::ShaderLanguage::GLSL, fyuu_rhi::ShaderCompilationOption{});
#elif defined(__linux__)
			std::visit(
				[context](auto&& handle) {
					using Handle = std::decay_t<decltype(handle)>;
					if constexpr (std::same_as<Handle, fyuu_rhi::Wayland>) {
						attachment->vertex_shader.emplace(*m_context.logical_device, vertex_shader_es_src, fyuu_rhi::ShaderStage::Vertex, fyuu_rhi::ShaderLanguage::GLSL, fyuu_rhi::ShaderCompilationOption{});
						attachment->fragment_shader.emplace(*m_context.logical_device, fragment_shader_es_src, fyuu_rhi::ShaderStage::Fragment, fyuu_rhi::ShaderLanguage::GLSL, fyuu_rhi::ShaderCompilationOption{});
					}
					else if constexpr (std::same_as<Handle, fyuu_rhi::Xlib>) {
						attachment->vertex_shader.emplace(*m_context.logical_device, opengl_vertex_shader_src, fyuu_rhi::ShaderStage::Vertex, fyuu_rhi::ShaderLanguage::GLSL, fyuu_rhi::ShaderCompilationOption{});
						attachment->fragment_shader.emplace(*m_context.logical_device, opengl_fragment_shader_src, fyuu_rhi::ShaderStage::Fragment, fyuu_rhi::ShaderLanguage::GLSL, fyuu_rhi::ShaderCompilationOption{});
					}
					else{

					}
				},
				platform_handle.impl
			)
#elif defined(__ANDROID__)
			attachment->vertex_shader.emplace(*m_context.logical_device, vertex_shader_es_src, fyuu_rhi::ShaderStage::Vertex, fyuu_rhi::ShaderLanguage::GLSL, fyuu_rhi::ShaderCompilationOption{});
			attachment->fragment_shader.emplace(*m_context.logical_device, fragment_shader_es_src, fyuu_rhi::ShaderStage::Fragment, fyuu_rhi::ShaderLanguage::GLSL, fyuu_rhi::ShaderCompilationOption{});
#endif // ! defined(_WIN32)
			segment.Declare(0u, 16u, fyuu_rhi::ShaderStage::Vertex)
				.Declare(fyuu_rhi::ResourceFlags::SampledTexture, 0u, 1u, fyuu_rhi::ShaderStage::Fragment)
				.Declare(
					fyuu_rhi::SamplerFlags::MagLinear | fyuu_rhi::SamplerFlags::MinLinear | fyuu_rhi::SamplerFlags::MipLinear | fyuu_rhi::SamplerFlags::ClampToEdge,
					{},
					1u,
					fyuu_rhi::ShaderStage::Fragment
				)
				.Instantiate();
			break;
		case fyuu_rhi::API::WebGPU:
			// TODO: WebGPU shader
#if defined(__APPLE__)
		case fyuu_rhi::API::Metal:
			// TODO: Metal shader
#endif // defined(__APPLE__)
		default:
			throw std::runtime_error("Not implemented");
		}
	}

	void ImGUI::CompileGPUExecutable() {
		
		auto& attachment = m_context.attachment;
		fyuu_rhi::GPUExecutable& gpu_executable = 
			attachment->gpu_executable ? *attachment->gpu_executable : attachment->gpu_executable.emplace(*m_context.logical_device);
			
		gpu_executable.Reset();

		switch (m_context.logical_device->GetAPI()) {
		case fyuu_rhi::API::Vulkan:
			gpu_executable.AddVertexBinding({ 0u, sizeof(ImDrawVert), fyuu_rhi::InputRate::Vertex })
				.AddVertexAttribute({ 0u, 0u, offsetof(ImDrawVert, pos), fyuu_rhi::ResourceFlags::RG32_Sfloat })
				.AddVertexAttribute({ 1u, 0u, offsetof(ImDrawVert, uv), fyuu_rhi::ResourceFlags::RG32_Sfloat })
				.AddVertexAttribute({ 2u, 0u, offsetof(ImDrawVert, col), fyuu_rhi::ResourceFlags::BGRA8_Unorm });
			break;

		case fyuu_rhi::API::DirectX12:
			gpu_executable.AddVertexBinding({ 0u, sizeof(ImDrawVert), fyuu_rhi::InputRate::Vertex })
				.AddVertexAttribute({ fyuu_rhi::SemanticInfo{ "POSITION", 0u }, 0u, offsetof(ImDrawVert, pos), fyuu_rhi::ResourceFlags::RG32_Sfloat })
				.AddVertexAttribute({ fyuu_rhi::SemanticInfo{ "COLOR", 0u }, 0u, offsetof(ImDrawVert, uv), fyuu_rhi::ResourceFlags::RG32_Sfloat })
				.AddVertexAttribute({ fyuu_rhi::SemanticInfo{ "TEXCOORD", 0u }, 0u, offsetof(ImDrawVert, col), fyuu_rhi::ResourceFlags::BGRA8_Unorm });		
			break;

		case fyuu_rhi::API::OpenGL:
			gpu_executable.AddVertexBinding({ 0u, sizeof(ImDrawVert), fyuu_rhi::InputRate::Vertex })
				.AddVertexAttribute({ 0u, 0u, offsetof(ImDrawVert, pos), fyuu_rhi::ResourceFlags::RG32_Sfloat })
				.AddVertexAttribute({ 1u, 0u, offsetof(ImDrawVert, uv), fyuu_rhi::ResourceFlags::RG32_Sfloat })
				.AddVertexAttribute({ 2u, 0u, offsetof(ImDrawVert, col), fyuu_rhi::ResourceFlags::BGRA8_Unorm });
			break;
		case fyuu_rhi::API::WebGPU:
			// TODO: WebGPU shader
#if defined(__APPLE__)
		case fyuu_rhi::API::Metal:
			// TODO: Metal shader
#endif // defined(__APPLE__)
		default:
			throw std::runtime_error("Not implemented");
		}		

		std::array blend_info = {
			fyuu_rhi::BlendInfo{
				fyuu_rhi::BlendFactor::SrcAlpha,
				fyuu_rhi::BlendFactor::OneMinusSrcAlpha,
				fyuu_rhi::BlendOp::Add,
				fyuu_rhi::BlendFactor::One,
				fyuu_rhi::BlendFactor::OneMinusSrcAlpha,
				fyuu_rhi::BlendOp::Add
			}
		};

		std::array color_write_masks = {
			std::uint8_t(0xFF)
		};

		std::array rt_formats = {
			fyuu_rhi::ResourceFlags::RGBA8_Unorm
		};

		static constexpr fyuu_rhi::GPUExecutableFlags flags = 
			fyuu_rhi::GPUExecutableFlagBits::Graphics |
			fyuu_rhi::GPUExecutableFlagBits::EnableDepthClip | 
			fyuu_rhi::GPUExecutableFlagBits::CullModeNone | 
			fyuu_rhi::GPUExecutableFlagBits::FillModeSolid | 
			fyuu_rhi::GPUExecutableFlagBits::PrimitiveTopologyTriangleList;

		gpu_executable.SetShader(fyuu_rhi::ShaderStage::Vertex, *attachment->vertex_shader)
			.SetShader(fyuu_rhi::ShaderStage::Fragment, *attachment->fragment_shader)
			.SetShaderDataSegment(*m_context.attachment->shader_data_segment)
			.SetRenderTargetAttachments(blend_info, color_write_masks)
			.SetFlags(flags)
			.SetRenderTargetFormats(rt_formats)
			.Compile();

	}

	void ImGUI::CreateFontTexture() {

		ImGuiIO& io = ImGui::GetIO();
		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		if (width < 0 || height < 0) {
			throw std::runtime_error("Invalid UI font texture");
		}

		auto& attachment = m_context.attachment;
		fyuu_rhi::Resource& font_texture = attachment->font_texture.emplace(
			*m_context.logical_device, 
			fyuu_rhi::TextureSize{ static_cast<std::size_t>(width), static_cast<std::size_t>(height), 1u }, 
			fyuu_rhi::VideoMemoryType::DeviceLocal,
			fyuu_rhi::ResourceFlags::SampledTexture | fyuu_rhi::ResourceFlags::RGBA8_Unorm
		);

		fyuu_rhi::Resource upload(
			*m_context.logical_device, 
			{ static_cast<std::size_t>(width), static_cast<std::size_t>(height), 1u }, 
			fyuu_rhi::VideoMemoryType::HostVisible,
			fyuu_rhi::ResourceFlags::SampledTexture | fyuu_rhi::ResourceFlags::RGBA8_Unorm
		);

		

	}

	void ImGUI::CreateResource() {

		auto api = m_context.logical_device->GetAPI();
		if (api == fyuu_rhi::API::OpenGL) {
			CompileShader();
			CompileGPUExecutable();
			CreateFontTexture();
			m_resource_ready.test_and_set(std::memory_order::release);
		} 
		else {
			auto tg = std::make_shared<tbb::task_group>();
			auto shader_promise = std::make_shared<std::promise<void>>();
			auto gpu_promise = std::make_shared<std::promise<void>>();
	
			tg->run(
				[this, shader_promise] {
					CompileShader();
					shader_promise->set_value();
				}
			);
	
			tg->run(
				[this, shader_promise, gpu_promise, tg] {
					shader_promise->get_future().wait();
					CompileGPUExecutable();
					gpu_promise->set_value();
				}
			);
	
			tg->run(
				[this, gpu_promise, tg] {
					gpu_promise->get_future().wait();
					CreateFontTexture();
					m_resource_ready.test_and_set(std::memory_order::release);
				}
			);
	
		}

	}

	ImGUI::ImGUI(Application const* application)
		: PolymorphicUIBase(this),
		m_resource_ready() {

		initialize();

		SetStyle(context.style);
		SetScale(context.scale);

		ImGuiIO& io = ImGui::GetIO();
		IMGUI_CHECKVERSION();
		IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");
		
		m_context.attachment = std::make_shared<Attachment>();

		io.BackendRendererUserData = &m_context;
		io.BackendRendererName = "imgui_impl_fyuu_rhi";

		CreateResource();

	}

	void imgui::ImGUI::SetStyle(Style style) {
		switch (style) {
		case Style::Dark:
			ImGui::StyleColorsDark();
			break;
		case Style::Light:
			ImGui::StyleColorsLight();
			break;
		default:
			ImGui::StyleColorsDark();
			break;
		}
	}

	void imgui::ImGUI::SetScale(float scale) {
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(scale);
	}

	void imgui::ImGUI::Shutdown() {
		m_context.attachment = nullptr;
	} */

	ImGUI::ImGUI(UIInit const& init)
		: PolymorphicUIBase(this),
		m_immutable_resource(nullptr) {

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		switch (style) {
		case Style::Dark:
			ImGui::StyleColorsDark();
			break;
		case Style::Light:
			ImGui::StyleColorsLight();
			break;
		default:
			throw std::invalid_argument("Unknown UI style");
		}

		// Setup scaling
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(init.scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
		style.FontScaleDpi = init.scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)
	
		void* surface = init.surface;

		switch (init.app_backend) {
		case application::ApplicationBackend::SDL3:
			ImGui_ImplSDL3_InitForOther(static_cast<SDL_Window*>(surface));
			break;
		case application::ApplicationBackend::GLFW:
			ImGui_ImplGlfw_InitForOther(static_cast<GLFWwindow*>(surface), false);
			break;
		default:
			throw std::invalid_argument("Unknown Application backend");
		}

	}

} // namespace fyuu_engine::ui::imgui
