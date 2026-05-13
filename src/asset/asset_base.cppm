module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <vector>
#include <string>
#include <string_view>
#include <atomic>
#include <chrono>
#include <filesystem>
#endif // !defined(__cpp_lib_modules)
#include <boost/uuid.hpp>
#include <boost/describe.hpp>
#include <boost/mp11.hpp>
export module fyuu_engine:asset_base;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
namespace fs = std::filesystem;
namespace fyuu_engine::asset {
	
	export using AssetID = boost::uuids::uuid;
	
	export enum class AssetType : std::size_t {
		Invalid,
		Bitmap,
	};

	export struct AssetBase {

		/*
			reflective fields
		*/

		AssetID id;
		AssetType type;
		std::chrono::steady_clock::time_point timestamp;
		std::vector<AssetID> dependency_ids;

		/*
			runtime fields
		*/
	
		fs::path conf_path;
		std::atomic_size_t ref_count;
		std::vector<AssetBase*> dependencies;

	};

} // namespace fyuu_engine::asset

export {
	BOOST_DESCRIBE_ENUM(fyuu_engine::asset::AssetType, Invalid)
	BOOST_DESCRIBE_STRUCT(fyuu_engine::asset::AssetBase, (), (id, type, timestamp, dependency_ids))
}

namespace fyuu_engine::serialization {

	export template <class T, class Serializer> void Serialize(std::string const& key, T const& val, Serializer& serializer) {
		if constexpr (requires{ serializer[key] = val; }) {
			serializer[key] = val;
		}
	}

	export template <class T, class Serializer> void Deserialize(std::string const& key, T& obj, Serializer const& serializer) {
		if constexpr (requires{ obj = serializer[key].template As<T>(); }) {
			obj = serializer[key].template As<T>();
		}
		else if constexpr (requires{ obj = serializer[key].template as<T>(); }) {
			obj = serializer[key].template as<T>();
		}
		else {

		}
	}
	
	// ----- std::vector -------------------------------------------------

	export template <class T, class Serializer> void Serialize(std::string const& key, std::vector<T> const& val, Serializer& serializer) {
		if constexpr (requires{ serializer[key].push_back(std::declval<T>()); }) {
			for (auto const& element : val) {
				serializer[key].push_back(element);
			}
		}
		else if constexpr (requires{ serializer[key].PushBack(std::declval<T>()); }) {
			for (auto const& element : val) {
				serializer[key].PushBack(element);
			}
		}
		else {

		}
	}

	export template <class T, class Serializer> void Deserialize(std::string const& key, std::vector<T>& obj, Serializer const& serializer) {

		obj.clear();

		if constexpr (requires{ serializer[key]; }) {
			
			auto const& arr = serializer[key];

			
			if constexpr (requires { obj = arr.template As<std::vector<T>>(); }) {
				obj = arr.template As<std::vector<T>>();
				return;
			}
			else if constexpr (requires { obj = arr.template as<std::vector<T>>(); }) {
				obj = arr.template as<std::vector<T>>();
				return;
			}
			else if constexpr (requires { obj = arr.template get<std::vector<T>>(); }) {
				obj = arr.template get<std::vector<T>>();
				return;
			}
		
			if constexpr (requires { arr.size(); } && requires { arr[0]; }) {
				obj.reserve(arr.size());
				for (std::size_t i = 0; i < arr.size(); ++i) {
					T element;
					Deserialize(arr[i], element);
					obj.push_back(std::move(element));
				}
			}
			else if constexpr (requires { arr.GetArray(); }) {
				auto const& array = arr.GetArray();
				obj.reserve(array.Size());
				for (auto const& v : array) {
					T element;
					Deserialize(v, element);
					obj.push_back(std::move(element));
				}
			}

		}

	}

	// ----- fs::path -------------------------------------------------

	export template <class Serializer> void Serialize(std::string const& key, fs::path const& val, Serializer& serializer) {
		if constexpr (requires { serializer[key] = val.string(); }) {
			serializer[key] = val.string();
		}
	}

	export template <class Serializer>
	void Deserialize(std::string const& key, fs::path& obj, Serializer const& serializer) {
		if constexpr (requires { serializer[key].template As<std::string>(); }) {
			obj = fs::path(serializer[key].template As<std::string>());
		}
		else if constexpr (requires { serializer[key].template as<std::string>(); }) {
			obj = fs::path(serializer[key].template as<std::string>());
		}
		else {

		}
	}

	// ----- boost::uuids::uuid (AssetID) -----------------------------

	export template <class Serializer> void Serialize(std::string const& key, boost::uuids::uuid const& val, Serializer& serializer) {
		if constexpr (requires { serializer[key] = boost::uuids::to_string(val); }) {
			serializer[key] = boost::uuids::to_string(val);
		}
	}

	export template <class Serializer> void Deserialize(std::string const& key, boost::uuids::uuid& obj, Serializer const& serializer) {
		if constexpr (requires { serializer[key].template As<std::string>(); }) {
			obj = boost::uuids::string_generator()(serializer[key].template As<std::string>());
		}
		else if constexpr (requires { serializer[key].template as<std::string>(); }) {
			obj = boost::uuids::string_generator()(serializer[key].template as<std::string>());
		}
		else {

		}
	}

	// ----- AssetType ------------------------------------------------

	export template <class Serializer> void Serialize(std::string const& key, asset::AssetType val, Serializer& serializer) {
		if constexpr (requires { serializer[key] = static_cast<std::size_t>(val); }) {
			serializer[key] = static_cast<std::size_t>(val);
		}
	}

	export template <class Serializer> void Deserialize(std::string const& key, asset::AssetType& obj, Serializer const& serializer) {
		if constexpr (requires { serializer[key].template As<std::size_t>(); }) {
			obj = static_cast<asset::AssetType>(serializer[key].template As<std::size_t>());
		}
		else if constexpr (requires { serializer[key].template as<std::size_t>(); }) {
			obj = static_cast<asset::AssetType>(serializer[key].template as<std::size_t>());
		}
		else {

		}
	}
	
	// ----- std::chrono::steady_clock::time_point --------------------

	export template <class Serializer> void Serialize(std::string const& key, std::chrono::steady_clock::time_point const& val, Serializer& serializer) {
		auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(val.time_since_epoch()).count();
		if constexpr (requires { serializer[key] = ns; }) {
			serializer[key] = ns;
		}
	}

	export template <class Serializer> void Deserialize(std::string const& key, std::chrono::steady_clock::time_point& obj, Serializer const& serializer) {
		if constexpr (requires { serializer[key].template As<std::int64_t>(); }) {
			obj = std::chrono::steady_clock::time_point(
				std::chrono::nanoseconds(serializer[key].template As<std::int64_t>())
			);
		}
		else if constexpr (requires { serializer[key].template as<std::int64_t>(); }) {
			obj = std::chrono::steady_clock::time_point(
				std::chrono::nanoseconds(serializer[key].template as<std::int64_t>())
			);
		}
		else {

		}
	}

}