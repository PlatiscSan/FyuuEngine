module;
#include <yaml-cpp/yaml.h>

export module config:yaml;
import :interface;
import :node;
import std;

export namespace fyuu_engine::core {

	class YAMLConfig : public IConfig {
	private:
		ConfigNode m_node;
		std::filesystem::path m_current_config;
		std::filesystem::file_time_type m_last_modified;

		/// @brief mutex for node
		std::atomic_flag m_mutex;

		static ConfigNode::ConfigValue ParseYAMLNode(YAML::Node const& yaml_node);
		static YAML::Node SerializeNumber(Number const& num);
		static YAML::Node SerializeConfigValue(ConfigNode::ConfigValue const& value);

	public:
		YAMLConfig() = default;
		YAMLConfig(YAMLConfig&& other) noexcept;
		YAMLConfig& operator=(YAMLConfig&& other) noexcept;

		ConfigNode& Node() noexcept override;

		bool Open(std::filesystem::path const& file_path) override;

		bool Reopen() override;

		bool Save() override;

		bool SaveAs(std::filesystem::path const& file_path) override;

	};

}