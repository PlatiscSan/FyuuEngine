module;
#include <yaml-cpp/yaml.h>

module config.yaml;
import string_converter;
import pointer_wrapper;

namespace fyuu_engine::config {

	void YAMLConfig::ProceedScalar(util::PointerWrapper<ConfigNode> const& node, std::string const& key, std::string const& value) {

		switch (util::StringConverter::InferType(value)) {
		case util::StringConverter::ValueType::Bool:
			(*node)[key].Set(util::StringConverter::Convert<bool>(value));
			break;
		case util::StringConverter::ValueType::Int:
			(*node)[key].Set(util::StringConverter::Convert<std::intmax_t>(value));
			break;
		case util::StringConverter::ValueType::Uint:
			(*node)[key].Set(util::StringConverter::Convert<std::uintmax_t>(value));
			break;
		case util::StringConverter::ValueType::Float:
			(*node)[key].Set(util::StringConverter::Convert<double>(value));
			break;
		default:
			(*node)[key].Set(value);
		}
	}

	void YAMLConfig::ProceedSequence(util::PointerWrapper<ConfigNode> const& node, std::string const& key, YAML::Node const& yaml_node) {

		if (!yaml_node.IsSequence()) {
			return;
		}

		ConfigNode::Value::Array array;
		for (auto const& yaml_item : yaml_node) {

			auto item = yaml_item.as<std::string>();
			switch (util::StringConverter::InferType(item)) {
			case util::StringConverter::ValueType::Bool:
				array.emplace_back(std::in_place_type<Number>, std::in_place_type<std::uintmax_t>, util::StringConverter::Convert<bool>(item));
				break;
			case util::StringConverter::ValueType::Int:
				array.emplace_back(util::StringConverter::Convert<std::intmax_t>(item));
				break;
			case util::StringConverter::ValueType::Uint:
				array.emplace_back(util::StringConverter::Convert<std::uintmax_t>(item));
				break;
			case util::StringConverter::ValueType::Float:
				array.emplace_back(util::StringConverter::Convert<double>(item));
				break;
			}

		}

		(*node)[key].Set(array);

	}

	void YAMLConfig::ProceedNumber(ConfigNode::Value const& val, YAML::Node& yaml_node, std::string const& key) {
		
		auto& number = val.Get<Number>();
		
		std::visit(
			[&yaml_node, &key](auto&& arithmetic) {
				using Arithmetic = std::decay_t<decltype(arithmetic)>;
				if constexpr (std::is_same_v<Arithmetic, std::monostate>) {

				}
				else {
					yaml_node[key].push_back(arithmetic);
				}
			},
			number
		);

	}

	void YAMLConfig::ProceedConfigArray(ConfigNode::Value const& val, YAML::Node& yaml_node, std::string const& key) {

		auto& array = val.Get<ConfigNode::Value::Array>();

		for (auto const& element : array) {
			std::visit(
				[&val, &yaml_node, &key](auto&& element) {
					using Element = std::decay_t<decltype(element)>;
					if constexpr (std::is_same_v<Number, Element>) {
						YAMLConfig::ProceedNumber(val, yaml_node, key);
					}
					else if constexpr (std::is_same_v<std::string, Element>) {
						yaml_node[key].push_back(element);
					}
					else {

					}
				},
				element
			);
		}

	}

	void YAMLConfig::ProceedConfigNode(ConfigNode::Value const& val, YAML::Node& yaml_node, std::string const& key, auto& stack) {
		auto& node = val.Get<ConfigNode>();
		YAML::Node next_yaml_node(YAML::NodeType::Map);
		yaml_node[key] = next_yaml_node;
		stack.push_back({ node.begin(),node.end(), next_yaml_node });
	}

	void YAMLConfig::ParseYAML(YAML::Node const& yaml_root, ConfigNode& root) {

		if (!yaml_root.IsMap()) {
			return;
		}

		struct StackFrame {
			std::string path;
			YAML::Node yaml_node;
			util::PointerWrapper<ConfigNode> config_node;
		};

		std::vector<StackFrame> stack;

		stack.push_back({ "", yaml_root, &root });

		while (!stack.empty()) {

			auto [current_path, current_yaml, current_node] = std::move(stack.back());
			stack.pop_back();

			if (!current_yaml.IsMap()) {
				continue;
			}

			for (auto const& yaml_pair : current_yaml) {
				auto key = yaml_pair.first.as<std::string>();
				YAML::Node const& value_node = yaml_pair.second;
				std::shared_ptr<ConfigNode> nested_config;

				switch (value_node.Type()) {
				case YAML::NodeType::Map:
					nested_config = std::make_shared<ConfigNode>();
					(*current_node)[key].Set(nested_config);

					stack.push_back({ key, value_node, nested_config });
					break;

				case YAML::NodeType::Scalar:
					YAMLConfig::ProceedScalar(current_node, key, value_node.as<std::string>());
					break;

				case YAML::NodeType::Sequence:
					YAMLConfig::ProceedSequence(current_node, key, value_node);
					break;

				default:
					break;
				}

			}

		}


	}

	void YAMLConfig::ParseConfig(ConfigNode const& root, YAML::Node& yaml_root) {

		struct FrameStack {
			ConfigNode::ConstIterator begin;
			ConfigNode::ConstIterator end;
			YAML::Node yaml_node;
		};

		std::vector<FrameStack> stack;
		stack.push_back({ root.begin(), root.end(), yaml_root });

		while (!stack.empty()) {

			auto [current_begin, current_end, yaml_node] = std::move(stack.back());
			stack.pop_back();

			for (auto& iter = current_begin; iter != current_end; ++iter) {

				auto& [key, val] = *iter;

				switch (val.GetStorageType()) {
				case ConfigNode::Value::StorageType::Number:
					yaml_node[key] = val.GetOr(0);
					break;

				case ConfigNode::Value::StorageType::String:
					yaml_node[key] = val.GetOr("");
					break;

				case ConfigNode::Value::StorageType::Array:
					YAMLConfig::ProceedConfigArray(val, yaml_node, key);
					break;

				case ConfigNode::Value::StorageType::Node:
					YAMLConfig::ProceedConfigNode(val, yaml_node, key, stack);
					break;

				default:
					break;
				}

			}


		}


	}

	void YAMLConfig::OpenImpl(std::filesystem::path const& file_path) {

		if (!std::filesystem::exists(file_path)) {
			throw std::runtime_error("path does not exists");
		}
		
		YAML::Node yaml_root = YAML::LoadFile(file_path.string());

		YAMLConfig::ParseYAML(yaml_root, m_root);
		
	}

	void YAMLConfig::SaveAsImpl(std::filesystem::path const& file_path) const {

		auto parent_path = file_path.parent_path();
		if (!parent_path.empty() && !std::filesystem::exists(parent_path)) {
			// Create parent directories if they don't exist
			std::filesystem::create_directories(parent_path);
		}

		YAML::Node yaml_config(YAML::NodeType::Map);
		YAMLConfig::ParseConfig(m_root, yaml_config);

		std::ofstream out(file_path);
		out << yaml_config;

	}

}