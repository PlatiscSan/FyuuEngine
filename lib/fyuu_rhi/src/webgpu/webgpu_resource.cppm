// webgpu_resource.cpp
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <optional>
#include <variant>
#include <span>
#endif
#include "webgpu.hpp"

export module fyuu_rhi:webgpu_resource;

#if defined(__cpp_lib_modules)
import std;
#endif
import :types;
import :webgpu_common;

namespace fyuu_rhi::webgpu {

	export class WebGPUResource
		: public PolymorphicResourceBase,
		  public WebGPUCommon<WebGPUResource> {
	private:
		struct BackBuffer {
			wgpu::Texture texture;
			TextureSize size;
			wgpu::TextureFormat format;
		};

		std::variant<std::monostate, wgpu::Buffer, wgpu::Texture, BackBuffer> m_handle;

	public:
		WebGPUResource(
			WebGPULogicalDevice const& device, 
			BufferSize buffer_size,
			VideoMemoryType type, 
			ResourceFlags flags,
			std::string_view debug_name = {}
		);
		WebGPUResource(
			WebGPULogicalDevice const& device, 
			TextureSize const& texture_size,
			VideoMemoryType type, 
			ResourceFlags flags,
			std::string_view debug_name = {}
		);

		WebGPUResource(
			wgpu::Texture back_buffer, 
			TextureSize const& texture_size,
			wgpu::TextureFormat format
		);
	};
	
} // namespace fyuu_rhi::webgpu