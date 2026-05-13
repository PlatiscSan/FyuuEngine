module;
#include <version>
#if !defined(__cpp_lib_modules)
#endif // !defined(__cpp_lib_modules)
#if defined(_WIN32)
#include <dxgi1_3.h>
#include <d3d12.h>
#include <wrl.h>
#endif // defined(_WIN32)
#include <tbb/concurrent_hash_map.h>
export module fyuu_rhi:d3d12_resource_tracker;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :d3d12_utility;

namespace {
	tbb::concurrent_hash_map<ID3D12Resource*, D3D12_RESOURCE_STATES> s_resource_states;
}

namespace fyuu_rhi::d3d12 {
	void RegisterResource(Microsoft::WRL::ComPtr<ID3D12Resource> const& res, D3D12_RESOURCE_STATES state) {
		Microsoft::WRL::ComPtr<ID3DDestructionNotifier> notifier;
		HRESULT result = res.As(&notifier);
		ThrowIfFailed(result);
		UINT cb_id;
		result = notifier->RegisterDestructionCallback(
			[](void* context) {
				auto res = static_cast<ID3D12Resource*>(context);
				s_resource_states.erase(res);
			}, 
			res.Get(), 
			&cb_id
		);
		ThrowIfFailed(result);
		s_resource_states.insert({ res.Get(), state });
	}

	void UpdateResourceState(ID3D12Resource* res, D3D12_RESOURCE_STATES state) {
		decltype(s_resource_states)::accessor a;
		if (s_resource_states.find(a, res)) {
			a->second = state;
		}
	}
}
#endif // defined(_WIN32)