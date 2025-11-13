export module json_config;
import <nlohmann/json.hpp>;
export import base_config;
import pointer_wrapper;
import std;

namespace fyuu_engine::config {

	export class JSONConfig : public BaseConfig<JSONConfig> {
		friend class BaseConfig<JSONConfig>;
	private:

		static void ProceedArray(util::PointerWrapper<ConfigNode>& current_node, std::string const& key, nlohmann::json const& value_node);

		static void ParseJSON(nlohmann::json const& json_root, ConfigNode& root);

		static void ProceedNumber(ConfigNode::Value const& val, nlohmann::json& json_node, std::string const& key);

		static void ProceedConfigArray(ConfigNode::Value const& val, nlohmann::json& json_node, std::string const& key);

		static void ProceedConfigNode(ConfigNode::Value const& val, nlohmann::json& json_node, std::string const& key, auto& stack) {
			auto& node = val.Get<ConfigNode>();
			json_node[key] = nlohmann::json::object();
			stack.push_back({ node.begin(),node.end(), &json_node[key] });
		}

		static void SerializeConfig(ConfigNode const& root, nlohmann::json& json_root);

		void OpenImpl(std::filesystem::path const& file_path);

		void SaveAsImpl(std::filesystem::path const& file_path) const;

		std::string DumpImpl() const;

		void ParseImpl(std::string_view dumped);

	};

}