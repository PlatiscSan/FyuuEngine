export module core:config_manager;
import std;
import config;
import application;

namespace fyuu_engine::core::resource {

	export class ConfigManager {
	private:
		std::variant<std::monostate, config::YAMLConfig, config::JSONConfig> m_config;

		std::filesystem::path Directory(std::string const& field1, std::string const& field2) const;

		template <class T>
		T Field(std::string const& field1, std::string const& field2) const {
			return std::visit(
				[&field1, &field2](auto&& config) -> T {
					using Config = std::decay_t<decltype(config)>;
					if constexpr (std::is_same_v<std::monostate, Config>) {
						return {};
					}
					else {
						return config[field1][field2].As<T>();
					}
				},
				m_config
			);
		}


		template <class T>
		T Field(std::string const& field1, std::string const& field2, std::string const& field3) const {
			return std::visit(
				[&field1, &field2, &field3](auto&& config) -> T {
					using Config = std::decay_t<decltype(config)>;
					if constexpr (std::is_same_v<std::monostate, Config>) {
						return {};
					}
					else {
						return config[field1][field2][field3].As<T>();
					}
				},
				m_config
			);
		}

	public:
		ConfigManager(std::filesystem::path const& config_path);
		std::filesystem::path RootDirectory() const;
		std::filesystem::path AssetDirectory() const;
		std::filesystem::path LogDirectory() const;
		std::filesystem::path GlobalRenderingSettings() const;

		std::string_view Title() const;
		std::uint32_t Width() const;
		std::uint32_t Height() const;

		application::CommandMap LoadCommandMap() const noexcept;

	};

}