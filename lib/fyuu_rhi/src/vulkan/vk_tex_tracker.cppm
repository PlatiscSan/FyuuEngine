module;
#include <version>
#if !defined(__cpp_lib_modules)

#endif // !defined(__cpp_lib_modules)
#if !defined(__APPLE__)

#endif //!defined(__APPLE__)
#include <tbb/concurrent_hash_map.h>
export module fyuu_rhi:vulkan_texture_tracker;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import vulkan;
namespace {
	tbb::concurrent_hash_map<vk::Image, vk::ImageLayout> s_layouts;
}

namespace fyuu_rhi::vulkan {
	void RegisterTexture(vk::Image const& tex) {
		s_layouts.insert({ tex, vk::ImageLayout::eUndefined });
	}

	void UpdateTextureLayout(vk::Image const& tex, vk::ImageLayout layout) {
		decltype(s_layouts)::accessor a;
		if (s_layouts.find(a, tex)) {
			a->second = layout;
		}
	}

	void UnregisterTexture(vk::Image const& tex) {
		s_layouts.erase(tex);
	}

}
#endif // !defined(__APPLE__)