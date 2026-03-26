module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <filesystem>
#include <string_view>
#include <span>
#endif // !defined(__cpp_lib_modules)
module fyuu_rhi:shader_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :shader;
import :enums;
import :types;
import :logical_device;
#if !defined(__APPLE__)
import :vulkan;
import :opengl;
#elif 
import :metal;
#endif // !defined(__APPLE__)
#if defined(_WIN32)
import :d3d12;
#endif // defined(_WIN32)
import :webgpu;

namespace fs = std::filesystem;

namespace fyuu_rhi {

	Shader::Shader(
		LogicalDevice const& logical_device,
		std::string_view src,
		ShaderStage stage,
		ShaderLanguage language,
		ShaderCompilationOption const& option
	) : m_impl(
			[&logical_device, src, stage, language, &option]() {
		        auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
					[src, stage, language, &option](vulkan::VulkanLogicalDevice const* dev) -> UniqueShader {
						return plastic::utility::MakeUnique<vulkan::VulkanShader>(*dev, src, stage, language, option);
					},
					[src, stage, language, &option](opengl::OpenGLLogicalDevice const* dev) -> UniqueShader {
						return plastic::utility::MakeUnique<opengl::OpenGLShader>(*dev, src, stage, language, option);
					},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
					[src, stage, language, &option](d3d12::D3D12LogicalDevice const* dev) -> UniqueShader {
						return plastic::utility::MakeUnique<d3d12::D3D12Shader>(*dev, src, stage, language, option);
					},
#endif // defined(_WIN32)
#if defined(__APPLE__)
					[src, stage, language, &option](metal::MetalLogicalDevice const* dev) -> UniqueShader {
						return plastic::utility::MakeUnique<metal::MetalShader>(*dev, src, stage, language, option);
					},
#endif
					[src, stage, language, &option](webgpu::WebGPULogicalDevice const* dev) -> UniqueShader {
						return plastic::utility::MakeUnique<webgpu::WebGPUShader>(*dev, src, stage, language, option);
					},
				};
					
				return logical_device.GetHandle()->Visit(visitor);

			}()) {

	}

	Shader::Shader(
		LogicalDevice const& logical_device,
		std::span<std::byte const> bytes
	) : m_impl(
			[&logical_device, bytes]() {
		        auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
					[bytes](vulkan::VulkanLogicalDevice const* dev) -> UniqueShader {
						return plastic::utility::MakeUnique<vulkan::VulkanShader>(*dev, bytes);
					},
					[bytes](opengl::OpenGLLogicalDevice const* dev) -> UniqueShader {
						return plastic::utility::MakeUnique<opengl::OpenGLShader>(*dev, bytes);
					},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
					[bytes](d3d12::D3D12LogicalDevice const* dev) -> UniqueShader {
						return plastic::utility::MakeUnique<d3d12::D3D12Shader>(*dev, bytes);
					},
#endif // defined(_WIN32)
#if defined(__APPLE__)
					[bytes](metal::MetalLogicalDevice const* dev) -> UniqueShader {
						return plastic::utility::MakeUnique<metal::MetalShader>(*dev, bytes);
					},
#endif
					[bytes](webgpu::WebGPULogicalDevice const* dev) -> UniqueShader {
						return plastic::utility::MakeUnique<webgpu::WebGPUShader>(*dev, bytes);
					},
				};
					
				return logical_device.GetHandle()->Visit(visitor);	

			}()) {

	}

	Shader::Shader(
		LogicalDevice const& logical_device,
		fs::path const& path,
		ShaderCompilationOption const& option
	): m_impl(
		[&logical_device, &path, &option]() {
		        auto visitor = plastic::utility::Overload{
#if !defined(__APPLE__)
					[&path, &option](vulkan::VulkanLogicalDevice const* dev) -> UniqueShader {
						return plastic::utility::MakeUnique<vulkan::VulkanShader>(*dev, path, option);
					},
					[&path, &option](opengl::OpenGLLogicalDevice const* dev) -> UniqueShader {
						return plastic::utility::MakeUnique<opengl::OpenGLShader>(*dev, path, option);
					},
#endif // !defined(__APPLE__)
#if defined(_WIN32)
					[&path, &option](d3d12::D3D12LogicalDevice const* dev) -> UniqueShader {
						return plastic::utility::MakeUnique<d3d12::D3D12Shader>(*dev, path, option);
					},
#endif // defined(_WIN32)
#if defined(__APPLE__)
					[&path, &option](metal::MetalLogicalDevice const* dev) -> UniqueShader {
						return plastic::utility::MakeUnique<metal::MetalShader>(*dev, path, option);
					},
#endif
					[&path, &option](webgpu::WebGPULogicalDevice const* dev) -> UniqueShader {
						return plastic::utility::MakeUnique<webgpu::WebGPUShader>(*dev, path, option);
					},
				};
					
				return logical_device.GetHandle()->Visit(visitor);	
		}()) {

	}

	PolymorphicShaderBase* Shader::GetHandle() const noexcept {
		return m_impl.get();
	}

}