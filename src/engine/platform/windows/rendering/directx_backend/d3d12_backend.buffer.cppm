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

export module d3d12_backend:buffer;
export import :heap_pool;
export import collective_resource;
import std;
import disable_copy;

namespace fyuu_engine::windows::d3d12 {
#ifdef WIN32

	export enum class HeapPoolType : std::uint8_t {
		Unknown,
		/// @brief 
		SmallBuffer,
		/// @brief 
		MediumBuffer,
		/// @brief 
		LargeBuffer,
		/// @brief 
		SmallTexture,
		/// @brief 
		MediumTexture,
		/// @brief 
		LargeTexture,
		/// @brief 
		RenderTarget,
		/// @brief 
		DepthStencil,
		/// @brief 
		Upload,
		/// @brief 
		ReadBack,
		/// @brief 
		Custom,
		Count
	};

	using HeapPoolSizeMap = std::unordered_map<HeapPoolType, std::pair<std::size_t, std::size_t>>;

	export class D3D12Buffer : public util::DisableCopy<D3D12Buffer> {
		friend class D3D12BufferCollector;
	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
		D3D12HeapChunk m_chunk;

	public:
		D3D12Buffer(Microsoft::WRL::ComPtr<ID3D12Resource> const& resource, std::optional<D3D12HeapChunk>&& chunk);
		void Update(std::span<std::byte> data);
		D3D12_GPU_VIRTUAL_ADDRESS GPUAddress() const noexcept;
		ID3D12Resource* Native() const noexcept;
		std::size_t Size() const noexcept;
	};

	export class D3D12BufferCollector {
	private:
		D3D12HeapPool* m_pool;

	public:
		D3D12BufferCollector(D3D12HeapPool* pool) noexcept;
		void operator()(D3D12Buffer&& buffer);
		void operator()(D3D12Buffer& buffer);

	};

	using UniqueD3D12Buffer = util::UniqueCollectiveResource<D3D12Buffer, D3D12BufferCollector>;
#endif
}