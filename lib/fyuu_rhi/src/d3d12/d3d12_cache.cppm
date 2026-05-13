module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <compare>
#endif // !defined(__cpp_lib_modules)
#if defined(_WIN32)
#include <dxgi1_3.h>
#include <d3d12.h>
#include <wrl.h>
#endif // defined(_WIN32)
#include <tbb/concurrent_hash_map.h>
export module fyuu_rhi:d3d12_cache;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :d3d12_utility;

namespace {

	struct MSAAQualityCacheKey {
        DXGI_FORMAT Format;
        UINT SampleCount;
        D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS Flags;
		std::strong_ordering operator<=>(MSAAQualityCacheKey const& other) const noexcept = default;

		struct Compare {
			static size_t hash(MSAAQualityCacheKey const& key) {
				size_t h1 = std::hash<DXGI_FORMAT>{}(key.Format);
				size_t h2 = std::hash<UINT>{}(key.SampleCount);
				size_t h3 = std::hash<UINT>{}(static_cast<UINT>(key.Flags));
				return h1 ^ (h2 << 1) ^ (h3 << 2);
			}
			static bool equal(MSAAQualityCacheKey const& lhs, MSAAQualityCacheKey const& rhs) {
				return lhs == rhs;
			}
		};
    };

	using DeviceMSAACache = tbb::concurrent_hash_map<MSAAQualityCacheKey, UINT, MSAAQualityCacheKey::Compare>;

	tbb::concurrent_hash_map<ID3D12Device*, DeviceMSAACache> s_device_msaa_cache;

}

namespace fyuu_rhi::d3d12 {

	void RegisterDevice(Microsoft::WRL::ComPtr<ID3D12Device> const& dev) {
		Microsoft::WRL::ComPtr<ID3DDestructionNotifier> notifier;
		HRESULT result = dev.As(&notifier);
		ThrowIfFailed(result);

        {
            typename decltype(s_device_msaa_cache)::accessor acc;
            if (!s_device_msaa_cache.find(acc, dev.Get())) {
                s_device_msaa_cache.insert(acc, std::make_pair(dev.Get(), DeviceMSAACache{}));
            }
        }

		UINT cb_id;
		result = notifier->RegisterDestructionCallback(
			[](void* context) {
				auto dev = static_cast<ID3D12Device*>(context);
                s_device_msaa_cache.erase(dev);
			}, 
			dev.Get(), 
			&cb_id
		);
		ThrowIfFailed(result);
	}

	UINT QueryQualityLevels(ID3D12Device* device, DXGI_FORMAT format, UINT sample_cnt, D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE) {
		
		DeviceMSAACache* inner_cache = nullptr;
        {
            typename decltype(s_device_msaa_cache)::accessor outerAcc;
            if (!s_device_msaa_cache.find(outerAcc, device)) {
                s_device_msaa_cache.insert(outerAcc, std::make_pair(device, DeviceMSAACache{}));
            }
            inner_cache = &outerAcc->second;
        }

        MSAAQualityCacheKey key{ format, sample_cnt, flags };
        {
            typename DeviceMSAACache::accessor inner_acc;
            if (inner_cache->find(inner_acc, key)) {
                return inner_acc->second;
            }
        }

        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS query = {};
        query.Format = format;
        query.SampleCount = sample_cnt;
        query.Flags = flags;
        query.NumQualityLevels = 0;

        HRESULT hr = device->CheckFeatureSupport(
            D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
            &query,
            sizeof(query)
        );
        UINT quality_lvls = SUCCEEDED(hr) ? query.NumQualityLevels : 0;

        {
            typename DeviceMSAACache::accessor inner_acc;
            inner_cache->insert(inner_acc, std::make_pair(key, quality_lvls));
        }

        return quality_lvls;
	}

}
#endif // defined(_WIN32)