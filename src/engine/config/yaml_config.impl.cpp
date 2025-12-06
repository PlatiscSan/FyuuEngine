module config:yaml;
import string_converter;
import pointer_wrapper;

namespace fyuu_engine::config {

	void YAMLConfig::ProceedScalar(ConfigNode& node, std::string const& key, std::string const& value) {

		switch (util::StringConverter::InferType(value)) {
		case util::StringConverter::ValueType::Bool:
			node[key].Set(util::StringConverter::Convert<bool>(value));
			break;
		case util::StringConverter::ValueType::Int:
			node[key].Set(util::StringConverter::Convert<std::intmax_t>(value));
			break;
		case util::StringConverter::ValueType::Uint:
			node[key].Set(util::StringConverter::Convert<std::uintmax_t>(value));
			break;
		case util::StringConverter::ValueType::Float:
			node[key].Set(util::StringConverter::Convert<double>(value));
			break;
		default:
			node[key].Set(value);
		}
	}

	void YAMLConfig::ProceedSequence(ConfigNode& node, std::string const& key, YAML::Node const& yaml_node) {

		if (!yaml_node.IsSequence()) {
			return;
		}

		ConfigNode::Value::Array array;
		for (auto const& yaml_item : yaml_node) {

			auto item = yaml_item.as<std::string>();
			switch (util::StringConverter::InferType(item)) {
			case util::StringConverter::ValueType::Bool:
				array.emplace_back(util::StringConverter::Convert<bool>(item));
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

		node[key].Set(std::move(array));

	}

	void YAMLConfig::ProceedArithmetic(ConfigNode::Value const& val, YAML::Node& yaml_node, std::string const& key) {
		
		ConfigNode::Value::Arithmetic const& arithmetic = val.AsArithmetic();
		
		if (arithmetic.HoldsBool()) {
			yaml_node[key].push_back(arithmetic.As<bool>());
		}
		else if (arithmetic.HoldsFloat()) {
			yaml_node[key].push_back(arithmetic.As<long double>());
		}
		else if (arithmetic.HoldsSigned()) {
			yaml_node[key].push_back(arithmetic.As<std::ptrdiff_t>());
		}
		else if (arithmetic.HoldsSigned()) {
			yaml_node[key].push_back(arithmetic.As<std::size_t>());
		}
		else {

		}

	}

	void YAMLConfig::ProceedConfigArray(ConfigNode::Value const& val, YAML::Node& yaml_node, std::string const& key) {

		auto& array = val.As<ConfigNode::Value::Array>();

		for (auto const& element : array) {

			if (element.HoldsArithmetic()) {
				YAMLConfig::ProceedArithmetic(val, yaml_node, key);
			}
			else if (element.HoldsString()) {
				yaml_node[key].push_back(element.AsString());;
			}
			else {

			}

		}

	}

	void YAMLConfig::ParseYAML(YAML::Node const& yaml_root, ConfigNode& root) {

		if (!yaml_root.IsMap()) {
			return;
		}

		struct StackFrame {
			std::string path;
			YAML::Node yaml_node;
			ConfigNode* config_node;
		};

		std::array<std::byte, 16384u> buffer{};
		std::pmr::monotonic_buffer_resource pool(
			buffer.data(),
			buffer.size(),
			std::pmr::null_memory_resource()
		);

		std::pmr::vector<StackFrame> stack(&pool);

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

				switch (value_node.Type()) {
				case YAML::NodeType::Map:
					stack.push_back({ key, value_node, &(*current_node)[key].AsNode()});
					break;

				case YAML::NodeType::Scalar:
					YAMLConfig::ProceedScalar(*current_node, key, value_node.as<std::string>());
					break;

				case YAML::NodeType::Sequence:
					YAMLConfig::ProceedSequence(*current_node, key, value_node);
					break;

				default:
					break;
				}

			}

		}


	}

	void YAMLConfig::SerializeConfig(ConfigNode const& root, YAML::Node& yaml_root) {

		struct FrameStack {
			ConfigNode::ConstIterator begin;
			ConfigNode::ConstIterator end;
			YAML::Node yaml_node;
		};

		std::array<std::byte, 8192u> buffer{};
		std::pmr::monotonic_buffer_resource pool(
			buffer.data(),
			buffer.size(),
			std::pmr::null_memory_resource()
		);

		std::pmr::vector<FrameStack> stack(&pool);
		stack.push_back({ root.begin(), root.end(), yaml_root });

		while (!stack.empty()) {

			auto [current_begin, current_end, yaml_node] = std::move(stack.back());
			stack.pop_back();

			for (auto& iter = current_begin; iter != current_end; ++iter) {

				auto& [key, val] = *iter;

				switch (val.GetStorageType()) {
					case ConfigNode::Value::StorageType::Arithmetic:
					yaml_node[key] = val.AsOr(0.0f);
					break;

				case ConfigNode::Value::StorageType::String:
					yaml_node[key] = val.AsOr(std::string());
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
		YAMLConfig::SerializeConfig(m_root, yaml_config);

		std::ofstream out(file_path);
		out << yaml_config;

	}

	std::string YAMLConfig::DumpImpl() const {

		YAML::Node yaml_config(YAML::NodeType::Map);
		YAMLConfig::SerializeConfig(m_root, yaml_config);

		YAML::Emitter emitter;
		emitter << yaml_config;

		if (!emitter.good()) {
			throw std::runtime_error("Failed to convert YAML node to string");
		}

		return emitter.c_str();

	}

	void YAMLConfig::ParseImpl(std::string_view dumped) {
		YAML::Node yaml_root = YAML::Load(dumped.data());
		YAMLConfig::ParseYAML(yaml_root, m_root);
	}

	YAMLConfig::YAMLConfig(std::filesystem::path const& file_path) {
		OpenImpl(file_path);
	}

}