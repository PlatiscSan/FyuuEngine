/* option_parser.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <iostream>
#include <utility>
#include <string>
#include <filesystem>
#include <variant>
#include <print>
#endif
#include <CLI/CLI.hpp>
module fyuu_engine:application_option_parser_impl;
#if defined(__cpp_lib_modules)
import std;
#endif
import :application_option_parser;
import :application;
import :configuration;
import :rhi_context;

namespace fs = std::filesystem;

namespace fyuu_engine::application {

	static void ToLower(std::string& str) {
		std::transform(
			str.begin(),
			str.end(),
			str.begin(),
			[](unsigned char c) -> unsigned char {
				return static_cast<unsigned char>(std::tolower(c));
			}
		);
	}

	ApplicationOptionParser::ApplicationOptionParser(int argc, char** argv)
		: m_backend(std::in_place_type<std::string>),
		m_api(std::in_place_type<std::string>),
		m_conf(std::in_place_type<fs::path>),
		m_software_fallback(false),
		m_help_requested(false),
		m_cli_app("App description") {

		m_cli_app.add_option(
			"--backend", std::get<std::string>(m_backend),
			"Application backend, can be SDL3, GLFW"
		);
		
		m_cli_app.add_option(
			"--API", std::get<std::string>(m_api),
			"Graphics API, can be Vulkan, D3D12, Metal, WebGPU, OpenGL"
		);

		m_cli_app.add_option(
			"--conf", std::get<fs::path>(m_conf),
			"Configuration file path (YAML or JSON)"
		)->default_str("./conf.yaml");

		m_cli_app.add_flag(
			"--allow-software-fallback", m_software_fallback,
			"If graphics device not available, use software fallback"
		);

		m_cli_app.allow_extras(false);

		try {		
			m_cli_app.parse(argc, argv);

			ToLower(std::get<std::string>(m_backend));
			ToLower(std::get<std::string>(m_api));
		}
		catch(CLI::CallForHelp const&) {
			m_help_requested = true;
		}
		catch(std::exception const&) {
			throw;
		}

	}

	bool ApplicationOptionParser::ShowHelp() const noexcept {

		if (m_help_requested) {
			std::cout << m_cli_app.help() << std::endl;
			return true;
		}
		return false;

	}

	template <class Result, class Pairs>
	static Result ParseImpl(Pairs&& pairs, std::string_view str) {

		for (auto const& [pair_str, pair_enum] : pairs) {
			if (pair_str == str) {
				return pair_enum;
			}
		}

		return Result{};

	}

	ApplicationBackend ApplicationOptionParser::ParseBackend() noexcept {

		static constexpr std::array backend_pairs = {
			std::pair<std::string_view, ApplicationBackend>("sdl3", ApplicationBackend::SDL3),
			std::pair<std::string_view, ApplicationBackend>("glfw", ApplicationBackend::GLFW)
		};

		auto overload = plastic::utility::Overload{
			[](std::monostate) { return ApplicationBackend::PlatformDefault; },
			[](ApplicationBackend backend) { 
				return backend; 
			},
			[this](std::string& backend) {

				std::string backend_str = std::move(backend);
				return m_backend.emplace<ApplicationBackend>(ParseImpl<ApplicationBackend>(backend_pairs, backend_str));

			}
		};

		return std::visit(overload, m_backend);

	}

	fyuu_rhi::API ApplicationOptionParser::ParseAPI() noexcept {

		static constexpr std::array api_pairs = {
			std::pair<std::string_view, fyuu_rhi::API>("vulkan", fyuu_rhi::API::Vulkan),
			std::pair<std::string_view, fyuu_rhi::API>("d3d12", fyuu_rhi::API::DirectX12),
			std::pair<std::string_view, fyuu_rhi::API>("metal", fyuu_rhi::API::Metal),
			std::pair<std::string_view, fyuu_rhi::API>("webgpu", fyuu_rhi::API::WebGPU),
			std::pair<std::string_view, fyuu_rhi::API>("opengl", fyuu_rhi::API::OpenGL)
		};

		auto overload = plastic::utility::Overload{
			[](std::monostate) { return fyuu_rhi::API::PlatformDefault; },
			[](fyuu_rhi::API api) { return api; },
			[this](std::string& api) {

				std::string api_str = std::move(api);
				return m_api.emplace<fyuu_rhi::API>(ParseImpl<fyuu_rhi::API>(api_pairs, api_str));

			}
		};

		return std::visit(overload, m_api);

	}

	configuration::Configuration& ApplicationOptionParser::ParseConfiguration() noexcept {

		auto overload = plastic::utility::Overload{
			[](std::monostate) -> configuration::Configuration& { throw std::runtime_error("No path or configuration"); },
			[](configuration::Configuration& conf) -> configuration::Configuration& { return conf; },
			[this](fs::path& path) -> configuration::Configuration& {
				return m_conf.emplace<configuration::Configuration>(path);
			}
		};

		return std::visit(overload, m_conf);

	}

	bool ApplicationOptionParser::IsSoftwareFallbackAllowed() const noexcept {
		return m_software_fallback;
	}

} // namespace fyuu_engine::application

