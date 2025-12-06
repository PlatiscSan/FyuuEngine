export module config:yaml;
import :base;
import pointer_wrapper;
import <yaml-cpp/yaml.h>;

namespace fyuu_engine::config {

	export class YAMLConfig : public BaseConfig<YAMLConfig> {
		friend class BaseConfig<YAMLConfig>;
	private:
		static void ProceedScalar(ConfigNode& node, std::string const& key, std::string const& value);

		static void ProceedSequence(ConfigNode& node, std::string const& key, YAML::Node const& yaml_node);

		static void ProceedArithmetic(ConfigNode::Value const& val, YAML::Node& yaml_node, std::string const& key);

		static void ProceedConfigArray(ConfigNode::Value const& val, YAML::Node& yaml_node, std::string const& key);

		static void ProceedConfigNode(ConfigNode::Value const& val, YAML::Node& yaml_node, std::string const& key, auto& stack) {
			auto& node = val.As<ConfigNode>();
			YAML::Node next_yaml_node(YAML::NodeType::Map);
			yaml_node[key] = next_yaml_node;
			stack.push_back({ node.begin(),node.end(), next_yaml_node });
		}

		static void ParseYAML(YAML::Node const& yaml_root, ConfigNode& root);

		static void SerializeConfig(ConfigNode const& root, YAML::Node& yaml_root);

		void OpenImpl(std::filesystem::path const& file_path);

		void SaveAsImpl(std::filesystem::path const& file_path) const;

		std::string DumpImpl() const;

		void ParseImpl(std::string_view dumped);

	public:
		YAMLConfig() = default;
		YAMLConfig(std::filesystem::path const& file_path);

	};

}