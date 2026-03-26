module;
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string>
#include <filesystem>
#endif // !defined(__cpp_lib_modules)
export module fyuu_engine:configuration;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :configuration_types;
import plastic.sealed_polymorphism;
import :json;
import :yaml;

namespace fs = std::filesystem;

namespace fyuu_engine::configuration {

	export class Proxy {
	private:
		PolymorphicProxyBase* m_impl;

	public:
		Proxy(PolymorphicProxyBase& impl);

		template <class T>
		T As() const {
			return m_impl->Visit(
				[](auto const* derived) {
					return derived->template As<T>();
				}
			);
		}

		bool HasValue() const;	

		template <class T>
		std::string AsOr(T default_value) const {
			return HasValue() ? As<T>() : default_value;
		}

		template <class T>
		Proxy& operator=(T&& value) {
			m_impl->Visit(
				[value](auto* derived) {
					(*derived) = value;
				}
			);
			return *this;
		}

		Proxy operator[](std::string const& key);

	};

	export class Configuration {
	private:
		plastic::utility::UniqueBase<PolymorphicConfigurationBase> m_impl;

	public:
		Configuration(fs::path const& path);

		Proxy operator[](std::string const& key);

		void Save() const;

		void SaveAs(fs::path const& new_path) const;

	};

}