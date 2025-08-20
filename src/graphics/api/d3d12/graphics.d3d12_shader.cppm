module;
#ifdef WIN32
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <comdef.h>
#include "d3dx12.h"
#endif // WIN32

#ifdef interface
#undef interface
#endif // interface

export module graphics:d3d12_shader;
export import :d3d12_renderer;
import std;

namespace graphics::api::d3d12 {
#ifdef WIN32
	class D3D12Shader : public IShader {
	private:
		std::string m_source;
		std::string m_entry_point;
		ShaderType m_type;
		Microsoft::WRL::ComPtr<ID3DBlob> m_shader;
		Microsoft::WRL::ComPtr<ID3DBlob> m_error_message;

	public:
		D3D12Shader(std::string_view source, std::string_view entry, ShaderType type);

		ShaderType GetType() const noexcept override;

		ShaderFormat GetFormat() const noexcept override;

		std::string_view GetSource() const noexcept override;

		std::string_view GetEntryPoint() const noexcept override;

		void Bind(BaseRenderDevice& renderer) const noexcept override;

		void Unbind() const noexcept override;

		void* GetNativeHandle() const noexcept override;

	};
#endif // WIN32
}