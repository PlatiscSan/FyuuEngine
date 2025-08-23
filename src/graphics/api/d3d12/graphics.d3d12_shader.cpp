module;
#ifdef WIN32
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <comdef.h>
#endif // WIN32

module graphics:d3d12_shader;

#ifdef WIN32

static inline void ThrowIfFailed(HRESULT hr) {
	if (FAILED(hr)) {
#ifdef _DEBUG
#ifdef UNICODE
		auto str = std::wformat(L"**ERROR** Fatal Error with HRESULT of {}", hr);
#else
		auto str = std::format("**ERROR** Fatal Error with HRESULT of {}", hr);
#endif
		OutputDebugString(str.data());
		__debugbreak();
#endif
		throw _com_error(hr);
	}
}

namespace graphics::api::d3d12 {

	static char const* GetShaderTargetProfile(ShaderType type) {
		switch (type) {
		case ShaderType::VERTEX:
			return "vs_5_0";
		case ShaderType::PIXEL:
			return "ps_5_0";
		case ShaderType::GEOMETRY:
			return "gs_5_0";
		case ShaderType::COMPUTE:
			return "cs_5_0";
		default:
			throw std::invalid_argument("Unknown shader type");
		}
	}

	D3D12Shader::D3D12Shader(std::string_view source, std::string_view entry, ShaderType type)
		: m_source(source), m_entry_point(entry), m_type(type) {
		HRESULT result = D3DCompile(
			source.data(),
			source.size(),
			nullptr,
			nullptr,
			nullptr,
			entry.data(), // Entry point
			GetShaderTargetProfile(type), // Target profile
			D3DCOMPILE_ENABLE_STRICTNESS, // Flags
			0, // Flags2
			&m_shader, // Output blob
			&m_error_message // Error messages
		);
		ThrowIfFailed(result);
	}

	ShaderType D3D12Shader::GetType() const noexcept {
		return m_type;
	}

	ShaderFormat D3D12Shader::GetFormat() const noexcept {
		return ShaderFormat::DXBC; // DirectX Bytecode
	}

	std::string_view D3D12Shader::GetSource() const noexcept {
		return m_source;
	}

	std::string_view D3D12Shader::GetEntryPoint() const noexcept {
		return m_entry_point;
	}

	void D3D12Shader::Bind(BaseRenderDevice& renderer) const noexcept {

		auto& d3d12_renderer = static_cast<D3D12RenderDevice&>(renderer);
		auto device = d3d12_renderer.GetD3D12Device();
		D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};

	}

	void D3D12Shader::Unbind() const noexcept
	{
	}

	void* D3D12Shader::GetNativeHandle() const noexcept
	{
		return nullptr;
	}

}
#endif // WIN32
