#ifndef TEXTURE_H
#define TEXTURE_H

#include "GPUResource.h"
#include "D3D12Core.h"

namespace Fyuu::graphics::d3d12 {

	class Texture2D : public GPUResource {

	public:

		Texture2D(std::string const& name, std::uint32_t width, std::uint32_t height, std::size_t row_pitch_bytes, DXGI_FORMAT format, void const* data);

		Microsoft::WRL::ComPtr<ID3D12Resource2> const& GetResource() const noexcept { return m_resource; }

	protected:

		std::string m_name;
		std::uint32_t m_width, m_height;

	};

}

#endif // !TEXTURE_H
