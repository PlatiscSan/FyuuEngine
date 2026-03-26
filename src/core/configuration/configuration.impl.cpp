module;
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <memory>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <format>
#endif // !defined(__cpp_lib_modules)
module fyuu_engine:configuration_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :configuration;
import :configuration_types;

import :json;
import :yaml;

namespace fs = std::filesystem;

namespace fyuu_engine::configuration {

	Proxy::Proxy(PolymorphicProxyBase& impl)
		: m_impl(&impl) {

	}

	bool Proxy::HasValue() const {
		return m_impl->Visit(
			[](auto const* derived) {
				return derived->HasValue();
			}
		);
	}

	Proxy Proxy::operator[](std::string const& key) {
		return m_impl->Visit(
			[&key](auto* derived) -> PolymorphicProxyBase& {
				return (*derived)[key];
			}
		);
	}

	Configuration::Configuration(fs::path const& path)
		: m_impl(
			[&path]() -> plastic::utility::UniqueBase<PolymorphicConfigurationBase> {
				std::ifstream file(path);
				if (!file.is_open()) {
					return plastic::utility::MakeUnique<yaml::YAML>(path);
				}
				std::stringstream buffer;
				buffer << file.rdbuf();
				std::string content = buffer.str();

				std::string ext = path.extension().string();
				if (ext == ".json") {
					return plastic::utility::MakeUnique<json::JSON>(content, path);
				}
				else if (ext == ".yaml" || ext == ".yml") {
					return plastic::utility::MakeUnique<yaml::YAML>(content, path);
				}
				else {
					throw std::runtime_error(std::format("Unsupported configuration file extension: {}", ext));
				}

			}()) {
	}

	Proxy Configuration::operator[](std::string const& key) {
		return m_impl->Visit(
			[&key](auto* derived) -> PolymorphicProxyBase& {
				return (*derived)[key];
			}
		);
	}

	void Configuration::Save() const {
		m_impl->Visit(
			[](auto const* derived) {
				derived->Save();
			}
		);
	}

	void Configuration::SaveAs(fs::path const& new_path) const {
		m_impl->Visit(
			[&new_path](auto const* derived) {
				derived->SaveAs(new_path);
			}
		);
	}


}
