module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <functional>
#include <filesystem>
#include <string_view>
#endif // !defined(__cpp_lib_modules)
export module fyuu_engine:asset_common;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :asset_base;
namespace fs = std::filesystem;

namespace {
	using namespace fyuu_engine::asset;
	fs::path s_asset_dir;
}

namespace fyuu_engine::asset {

	export std::function<void(std::string_view)> Warning;
	export std::function<void(std::string_view)> Error;

	export fs::path ResolveFullPath(fs::path const& path) {

        if (path.is_absolute()) {
			return path;
		}

		fs::path full_path = ".";
		full_path /= (s_asset_dir / path);
		return full_path;

	}

	export void SetAssetRootDir(fs::path const& dir) {
		s_asset_dir = dir;
	}

}