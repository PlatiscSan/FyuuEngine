export module config.json;
export import <nlohmann/json.hpp>;
export import config;
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
			nlohmann::json next_json_node = nlohmann::json::object();
			json_node[key] = next_json_node;
			stack.push_back({ node.begin(),node.end(), next_json_node });
		}

		static void SerializeConfig(ConfigNode const& root, nlohmann::json& json_root);

		void OpenImpl(std::filesystem::path const& file_path);

		void SaveAsImpl(std::filesystem::path const& file_path) const;

		std::string ToStringImpl() const;

	};

}