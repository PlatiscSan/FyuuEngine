module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <unordered_map>
#include <memory>
#include <memory_resource>
#include <filesystem>
#endif
#include <yaml-cpp/yaml.h>
export module fyuu_engine:yaml;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :configuration_types;

namespace fs = std::filesystem;

namespace fyuu_engine::configuration::yaml {

    export class YAMLProxy 
		: public PolymorphicProxyBase {
	private:
		std::pmr::unordered_map<std::string, std::unique_ptr<YAMLProxy>> m_map;
		::YAML::Node m_node;

	public:
		YAMLProxy(std::pmr::memory_resource* pool, ::YAML::Node node);
		YAMLProxy& operator[](std::string const& key);

		template <class T>
		std::string As() const {
			return m_node.as<T>();
		}

		bool HasValue() const;

		template <class T>
		YAMLProxy& operator=(T&& value) {
			m_node = value;
			return *this;
		}

	};

	export class YAML 
		: public PolymorphicConfigurationBase {
	private:
		std::pmr::synchronized_pool_resource m_pool;
		::YAML::Node m_root;
		YAMLProxy m_root_proxy;
		fs::path m_path;

	public:
		YAML(fs::path const& path = {});
		YAML(std::string const& yaml_str, fs::path const& path = {});

		YAMLProxy& operator[](std::string const& key);

		void Save() const;
		void SaveAs(fs::path const& new_path) const;
	};

}
