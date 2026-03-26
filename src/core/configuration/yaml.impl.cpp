module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <fstream>
#include <unordered_map>
#include <memory>
#include <memory_resource>
#include <filesystem>
#endif
#include <yaml-cpp/yaml.h>
module fyuu_engine:yaml_impl;
import :yaml;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :configuration_types;

namespace fs = std::filesystem;

namespace fyuu_engine::configuration::yaml {

	YAMLProxy::YAMLProxy(std::pmr::memory_resource* pool, ::YAML::Node node)
		: PolymorphicProxyBase(this), m_map(pool), m_node(node) {
	}

	YAMLProxy& YAMLProxy::operator[](std::string const& key) {
		auto it = m_map.find(key);
		if (it == m_map.end()) {

			if (!m_node[key].IsDefined()) {
				m_node[key] = ::YAML::Node(::YAML::NodeType::Null);
			}

			auto [new_it, _] = m_map.emplace(
				std::pmr::string(key, m_map.get_allocator().resource()),
				std::make_unique<YAMLProxy>(m_map.get_allocator().resource(), m_node[key])
			);
			it = new_it;
		}
		return *it->second;
	}
	
	bool YAMLProxy::HasValue() const {
		return m_node.IsDefined() && !m_node.IsNull();
	}

}

namespace fyuu_engine::configuration::yaml {

    YAML::YAML(fs::path const& path)
        : PolymorphicConfigurationBase(this),
        m_pool(std::pmr::pool_options{ 16, 256 }),
        m_root(::YAML::NodeType::Null),
        m_root_proxy(&m_pool, m_root),
        m_path(path) {
    }

    YAML::YAML(std::string const& yaml_str, fs::path const& path)
        : PolymorphicConfigurationBase(this),
        m_pool(std::pmr::pool_options{ 16, 256 }),
        m_root(::YAML::Load(yaml_str)),
        m_root_proxy(&m_pool, m_root),
        m_path(path) {
    }

    YAMLProxy& YAML::operator[](std::string const& key) {
        return m_root_proxy[key];
    }

    void YAML::Save() const {
        if (m_path.empty())
            throw std::runtime_error("No file path associated with configuration");
        std::ofstream file(m_path);
        if (!file)
            throw std::runtime_error(std::format("Cannot open file for writing: {}", m_path.string()));
        file << m_root;
    }

    void YAML::SaveAs(std::filesystem::path const& new_path) const {
        std::ofstream file(new_path);
        if (!file)
            throw std::runtime_error(std::format("Cannot open file for writing: {}", m_path.string()));
        file << m_root;
    }

}