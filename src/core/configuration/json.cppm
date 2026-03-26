module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <unordered_map>
#include <memory>
#include <memory_resource>
#include <filesystem>
#endif
export module fyuu_engine:json;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import nlohmann.json;
import :configuration_types;

namespace fs = std::filesystem;

namespace fyuu_engine::configuration::json {

	export class JSONProxy 
		: public PolymorphicProxyBase {
	private:
		std::pmr::unordered_map<std::string, std::unique_ptr<JSONProxy>> m_map;
		nlohmann::json m_j;

	public:
		JSONProxy(std::pmr::memory_resource* pool, nlohmann::json& j);
		JSONProxy& operator[](std::string const& key);

		template <class T>
		T As() const {
			return m_j.get<T>();
		}

		bool HasValue() const;

		template <class T>
		JSONProxy& operator=(T&& value) {
			m_j = value;
			return *this;
		}

	};

	export class JSON
		: public PolymorphicConfigurationBase {
	private:
		std::pmr::synchronized_pool_resource m_pool;
		nlohmann::json m_root;
		JSONProxy m_root_proxy;
		fs::path m_path;

	public:
		JSON(std::string const& json_str, fs::path const& path = {});

		JSONProxy& operator[](std::string const& key);

		void Save() const;
		void SaveAs(fs::path const& new_path) const;
	};

}