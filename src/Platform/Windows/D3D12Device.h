#ifndef D3D12_DEVICE_H
#define D3D12_DEVICE_H

#include "Framework/Renderer/GraphicsDevice.h"
#include "D3D12Utility.h"

namespace Fyuu {

	class D3D12Device final : public GraphicsDevice {

	public:

		D3D12Device();
		ID3D12Device10* GetID3D12Device() const noexcept {
			return m_device.Get();
		}

	private:

		Microsoft::WRL::ComPtr<ID3D12Device10> m_device;


	};

}

#endif // !D3D12_DEVICE_H
