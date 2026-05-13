module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <vector>
#include <string>
#include <string_view>
#include <span>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <filesystem>
#endif // !defined(__cpp_lib_modules)

#include <boost/uuid/uuid.hpp>
#include <boost/describe.hpp>
#include <boost/mp11.hpp>

#include <boost/gil.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/bmp.hpp>
//#include <boost/gil/extension/io/tiff.hpp>

export module fyuu_engine:bitmap_asset;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :asset_base;
import :asset_common;

namespace fs = std::filesystem;
namespace gil = boost::gil;

namespace {

	//using namespace fyuu_engine::asset;

	//gil::rgba8_image_t LoadImage(fs::path const& path) {
	//	gil::rgba8_image_t img;
	//	std::string ext = path.extension().string();
	//	if (ext == ".png") {
	//		gil::read_and_convert_image(path.string(), img, gil::png_tag{});
	//	} else if (ext == ".jpg" || ext == ".jpeg") {
	//		gil::read_and_convert_image(path.string(), img, gil::jpeg_tag{});
	//	} else if (ext == ".bmp") {
	//		gil::read_and_convert_image(path.string(), img, gil::bmp_tag{});
	//	} else if (ext == ".tif" || ext == ".tiff") {
	//		gil::read_and_convert_image(path.string(), img, gil::tiff_tag{});
	//	} else {
	//		std::string msg = std::format("LoadImage(): Unsupported image format '{}'", ext);
	//		throw std::runtime_error(msg);
	//	}
	//	return img;
	//}
	//
	//void SaveImage(gil::rgba8_image_t const& img, fs::path const& path) {
	//	std::string ext = path.extension().string();
	//	if (ext == ".png") {
	//		gil::write_view(path.string(), gil::const_view(img), gil::png_tag{});
	//	} else if (ext == ".jpg" || ext == ".jpeg") {
	//		gil::write_view(path.string(), gil::const_view(img), gil::jpeg_tag{});
	//	} else if (ext == ".bmp") {
	//		gil::write_view(path.string(), gil::const_view(img), gil::bmp_tag{});
	//	} else if (ext == ".tif" || ext == ".tiff") {
	//		gil::write_view(path.string(), gil::const_view(img), gil::tiff_tag{});
	//	} else {
	//		std::string msg = std::format("SaveImage(): Unsupported image format '{}'", ext);
	//		throw std::runtime_error(msg);
	//	}
	//}


}

namespace fyuu_engine::asset {

	//export struct RGBA8 {
	//	std::uint8_t r, g, b, a;
	//};

	//export struct Bitmap : public AssetBase {

	//	/*
	//		reflective fields
	//	*/

	//	fs::path src;

	//	/*
	//		runtime fields
	//	*/

	//	gil::rgba8_image_t image;
	//	mutable std::shared_mutex image_mutex;

	//	void Initialize() {
	//		if (src.empty()) {
	//			return;
	//		}
	//		fs::path full_path = ResolveFullPath(src);
	//		if (!fs::exists(full_path)) {
	//			std::string msg = std::format("Bitmap::Initialize(): Image file '{}' not found", full_path.string());
	//			throw std::runtime_error(msg);
	//		}
	//		std::unique_lock lock(image_mutex);
	//		image = LoadImage(full_path);
	//	}

	//	void Finalize() {
	//		std::unique_lock lock(image_mutex);
	//		image = gil::rgba8_image_t{};
	//	}

	//	void Save() {
	//		if (src.empty()) {
	//			throw std::runtime_error("Bitmap::Save(): Cannot save Bitmap, src path is empty");
	//		}
	//		fs::path full_path = ResolveImagePath(conf_path.parent_path(), src);
	//		std::shared_lock lock(image_mutex);
	//		SaveImage(image, full_path);
	//	}

	//	std::pair<std::unique_lock<std::shared_mutex>, std::span<RGBA8>> AsRGBA8() {
	//		std::unique_lock lock(image_mutex);
	//		auto view = gil::view(image);
	//		auto* raw_ptr = gil::interleaved_view_get_raw_data(view);
	//		std::size_t pixel_count = image.width() * image.height();
	//		return { std::move(lock), std::span<RGBA8>(reinterpret_cast<RGBA8*>(raw_ptr), pixel_count) };
	//	}
	//	
	//	std::pair<std::shared_lock<std::shared_mutex>, std::span<RGBA8 const>> AsRGBA8() const {
	//		std::shared_lock lock(image_mutex);
	//		auto view = gil::const_view(image);
	//		auto* raw_ptr = gil::interleaved_view_get_raw_data(view);
	//		std::size_t pixel_count = image.width() * image.height();
	//		return { std::move(lock), std::span<RGBA8 const>(reinterpret_cast<RGBA8 const*>(raw_ptr), pixel_count) };
	//	}

	//};

} // namespace fyuu_engine::asset

export {
	//BOOST_DESCRIBE_STRUCT(fyuu_engine::asset::Bitmap, (fyuu_engine::asset::AssetBase), (src))
}
