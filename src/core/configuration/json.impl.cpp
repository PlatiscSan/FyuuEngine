module;
#include <stdexcept>
#include <fstream>
#include <unordered_map>
#include <memory>
#include <memory_resource>
#include <filesystem>
module fyuu_engine:json_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :json;
import nlohmann.json;
import :configuration_types;

namespace fs = std::filesystem;

namespace fyuu_engine::configuration::json {

	JSONProxy::JSONProxy(std::pmr::memory_resource* pool, nlohmann::json& j)
		: PolymorphicProxyBase(this), m_map(pool), m_j(j) {
	}

	JSONProxy& JSONProxy::operator[](std::string const& key) {
		auto it = m_map.find(key);
		if (it == m_map.end()) {

			if (!m_j[key]) {
				m_j[key] = nlohmann::json();
			}

			auto [new_it, _] = m_map.emplace(
				std::pmr::string(key, m_map.get_allocator().resource()),
				std::make_unique<JSONProxy>(m_map.get_allocator().resource(), m_j[key])
			);
			it = new_it;
		}
		return *it->second;
	}

	bool JSONProxy::HasValue() const {
		return m_j;
	}

}

namespace fyuu_engine::configuration::json {

    JSON::JSON(std::string const& json_str, fs::path const& path)
        : PolymorphicConfigurationBase(this),
        m_pool(std::pmr::pool_options{ 16, 256 }),
        m_root(nlohmann::json::parse(json_str)),
        m_root_proxy(&m_pool, m_root),
        m_path(path) {
    }

    JSONProxy& JSON::operator[](std::string const& key) {
        return m_root_proxy[key];
    }

    void JSON::Save() const {
        if (m_path.empty())
            throw std::runtime_error("No file path associated with configuration");
        std::ofstream file(m_path);
        if (!file)
            throw std::runtime_error(std::format("Cannot open file for writing: {}", m_path.string()));
        file << m_root.dump(4);
    }

    void JSON::SaveAs(fs::path const& new_path) const {
        std::ofstream file(new_path);
        if (!file)
            throw std::runtime_error(std::format("Cannot open file for writing: {}", m_path.string()));
        file << m_root.dump(4);
    }

}