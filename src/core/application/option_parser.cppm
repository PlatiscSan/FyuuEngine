/* option_parser.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <iostream>
#include <utility>
#include <string>
#include <filesystem>
#include <variant>
#endif
#include <CLI/CLI.hpp>
export module fyuu_engine:application_option_parser;
#if defined(__cpp_lib_modules)
import std;
#endif
import :application;
import :configuration;
import :rhi_context;

namespace fs = std::filesystem;

namespace fyuu_engine::application {
	
	export class ApplicationOptionParser {
	private:
		std::variant<std::monostate, std::string, ApplicationBackend> m_backend;
		std::variant<std::monostate, std::string, fyuu_rhi::API> m_api;
		std::variant<std::monostate, fs::path, configuration::Configuration> m_conf;
		bool m_software_fallback;
		bool m_help_requested;
		CLI::App m_cli_app;
		
	public:
		ApplicationOptionParser(int argc, char** argv);

		bool ShowHelp() const noexcept;

		ApplicationBackend ParseBackend() noexcept;

		fyuu_rhi::API ParseAPI() noexcept;

		configuration::Configuration& ParseConfiguration() noexcept;

		bool IsSoftwareFallbackAllowed() const noexcept;

	};

} // namespace fyuu_engine::application
