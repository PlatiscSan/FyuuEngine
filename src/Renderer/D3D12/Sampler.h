#ifndef SAMPLER_H
#define SAMPLER_H

#include "D3D12Core.h"
#include "Framework/Core/Color.h"

namespace Fyuu::graphics::d3d12 {

	struct SamplerDesc : public D3D12_SAMPLER_DESC {

        //  These defaults match the default values for HLSL-defined root signature static samplers.  
        //  So not overriding them here means you can safely not define them in HLSL.
		SamplerDesc() {

            Filter = D3D12_FILTER_ANISOTROPIC;
            AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            MipLODBias = 0.0f;
            MaxAnisotropy = 16;
            ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
            BorderColor[0] = 1.0f;
            BorderColor[1] = 1.0f;
            BorderColor[2] = 1.0f;
            BorderColor[3] = 1.0f;
            MinLOD = 0.0f;
            MaxLOD = D3D12_FLOAT32_MAX;

		}

        void SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE address_mode) {
            AddressU = address_mode;
            AddressV = address_mode;
            AddressW = address_mode;
        }

        void SetBorderColor(core::Color border) {
            BorderColor[0] = border[core::Color::R];
            BorderColor[1] = border[core::Color::G];
            BorderColor[2] = border[core::Color::B];
            BorderColor[3] = border[core::Color::A];
        }

        // Allocate new descriptor as needed; return handle to existing descriptor when possible
        D3D12_CPU_DESCRIPTOR_HANDLE CreateDescriptor() const;

        // Create descriptor in place (no deduplication).  Handle must be preallocated
        void CreateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle);

	};

    enum SamplerType {
        SAMPLER_TYPE_NULL,
        SAMPLER_LINEAR_WRAP,
        SAMPLER_ANISO_WRAP,
        SAMPLER_SHADOW,
        SAMPLER_LINEAR_CLAMP,
        SAMPLER_VOLUME_WRAP,
        SAMPLER_POINT_CLAMP,
        SAMPLER_LINEAR_BORDER,
        SAMPLER_POINT_BORDER,
    };

    struct Sampler {
        SamplerDesc desc;
        D3D12_CPU_DESCRIPTOR_HANDLE handle;
    };

    void InitializeD3D12Samplers();

}

#endif // !SAMPLER_H
