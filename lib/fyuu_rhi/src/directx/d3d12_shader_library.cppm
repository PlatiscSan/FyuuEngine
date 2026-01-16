module;
#ifdef WIN32
#include <dxgi1_6.h>
#include <D3D12MemAlloc.h>
#include <directx/d3dx12.h>
#include <DescriptorHeap.h>
#include <wrl/client.h>
#include <comdef.h>
#endif // WIN32

export module fyuu_rhi:d3d12_shader_library;
import std;
import :pipeline;
import :d3d12_common;
import plastic.any_pointer;

namespace fyuu_rhi::d3d12 {

	export class D3D12ShaderLibrary
		: public plastic::utility::EnableSharedFromThis<D3D12ShaderLibrary>,
		public IShaderLibrary {
	private:
		Microsoft::WRL::ComPtr<ID3DBlob> m_shader_blob;
		ShaderReflection m_reflection;

	public:
		D3D12ShaderLibrary(std::filesystem::path const& path, plastic::utility::AnyPointer<CompilationOptions> const& options);
		D3D12ShaderLibrary(
			plastic::utility::AnyPointer<D3D12LogicalDevice> const& logical_device,
			std::string_view src, 
			ShaderStage shader_stage, 
			ShaderLanguage language, 
			plastic::utility::AnyPointer<CompilationOptions> const& options
		);

		D3D12ShaderLibrary(
			D3D12LogicalDevice const& logical_device,
			std::string_view src,
			ShaderStage shader_stage,
			ShaderLanguage language,
			CompilationOptions const& options
		);

		D3D12ShaderLibrary(std::span<std::byte const> compiled_bytes);
	};

}
