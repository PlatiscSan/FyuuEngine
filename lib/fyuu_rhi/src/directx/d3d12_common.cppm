module;
#ifdef WIN32
#include <Windows.h>
#endif // WIN32

export module fyuu_rhi:d3d12_common;
import std;

namespace fyuu_rhi::d3d12 {
#ifdef WIN32

	export class D3D12PhysicalDevice;
	export class D3D12LogicalDevice;
	export class D3D12CommandQueue;
	export class D3D12SwapChain;
	export class D3D12Fence;
	export class D3D12Texture;
	export class D3D12ShaderLibrary;
	export class D3D12VideoMemory;
	export class D3D12Resource;
	export class D3D12PipelineLayout;

	using EventHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(&CloseHandle)>;	

	export extern void ThrowIfFailed(HRESULT result);
	export extern EventHandle CreateEventHandle();
#endif // WIN32
}