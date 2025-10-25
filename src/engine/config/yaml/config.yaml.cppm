module;
#include <yaml-cpp/yaml.h>

export module config.yaml;
export import config;
import pointer_wrapper;

namespace fyuu_engine::config {

	export class YAMLConfig : public BaseConfig<YAMLConfig> {
		friend class BaseConfig<YAMLConfig>;
	private:
		static void ProceedScalar(util::PointerWrapper<ConfigNode> const& node, std::string const& key, std::string const& value);
		static void ProceedSequence(util::PointerWrapper<ConfigNode> const& node, std::string const& key, YAML::Node const& yaml_node);

		static void ProceedNumber(ConfigNode::Value const& val, YAML::Node& yaml_node, std::string const& key);
		static void ProceedConfigArray(ConfigNode::Value const& val, YAML::Node& yaml_node, std::string const& key);
		static void ProceedConfigNode(ConfigNode::Value const& val, YAML::Node& yaml_node, std::string const& key, auto& stack);

		static void ParseYAML(YAML::Node const& yaml_root, ConfigNode& root);

		static void ParseConfig(ConfigNode const& root, YAML::Node& yaml_root);

		void OpenImpl(std::filesystem::path const& file_path);

		void SaveAsImpl(std::filesystem::path const& file_path) const;
	};

}