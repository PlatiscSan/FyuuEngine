module config:json;
import string_converter;

namespace fyuu_engine::config {

	void JSONConfig::ProceedArray(
		util::PointerWrapper<ConfigNode>& current_node, 
		std::string const& key, 
		nlohmann::json const& value_node
	) {

		ConfigNode::Value::Array array;
		for (auto const& json_item : value_node) {
			if (json_item.is_string()) {
				array.emplace_back(json_item.get<std::string>());
			}
			else if (json_item.is_boolean()) {
				array.emplace_back(json_item.get<bool>());
			}
			else if (json_item.is_number_integer()) {
				array.emplace_back(json_item.get<std::ptrdiff_t>());
			}
			else if (json_item.is_number_unsigned()) {
				array.emplace_back(json_item.get<std::size_t>());
			}
			else if (json_item.is_number_float()) {
				array.emplace_back(json_item.get<double>());
			}
		}

		(*current_node)[key].Set(std::move(array));

	}

	void JSONConfig::ParseJSON(nlohmann::json const& json_root, ConfigNode& root) {

		if (json_root.empty()) {
			return;
		}

		struct StackFrame {
			std::string path;
			nlohmann::json const* json_node;
			util::PointerWrapper<ConfigNode> config_node;
		};

		std::array<std::byte, 8192u> buffer{};
		std::pmr::monotonic_buffer_resource pool(
			buffer.data(),
			buffer.size(),
			std::pmr::null_memory_resource()
		);

		std::pmr::vector<StackFrame> stack(&pool);

		stack.push_back({ "", &json_root, &root });

		while (!stack.empty()) {

			auto [current_path, current_json, current_node] = std::move(stack.back());
			stack.pop_back();

			if (!json_root.is_object()) {
				return;
			}

			for (auto const& json_pair : current_json->items()) {

				std::string const& key = json_pair.key();
				nlohmann::json const& value_node = json_pair.value();

				switch (value_node.type()) {
				case nlohmann::json::value_t::object:
					stack.push_back({ key, &value_node, &(*current_node)[key].AsNode() });
					break;

				case nlohmann::json::value_t::array:
					JSONConfig::ProceedArray(current_node, key, value_node);
					break;

				case nlohmann::json::value_t::string:
					(*current_node)[key].Set(value_node.get<std::string>());
					break;

				case nlohmann::json::value_t::boolean:
					(*current_node)[key].Set(value_node.get<bool>());
					break;

				case nlohmann::json::value_t::number_integer:
					(*current_node)[key].Set(value_node.get<std::ptrdiff_t>());
					break;

				case nlohmann::json::value_t::number_unsigned:
					(*current_node)[key].Set(value_node.get<std::size_t>());
					break;

				case nlohmann::json::value_t::number_float:
					(*current_node)[key].Set(value_node.get<double>());
					break;

				default:
					break;
				}

			}

		}


	}

	void JSONConfig::ProceedArithmetic(ConfigNode::Value const& val, nlohmann::json& json_node, std::string const& key) {
		
		ConfigNode::Value::Arithmetic const& arithmetic = val.AsArithmetic();
		if (arithmetic.HoldsBool()) {
			json_node[key] = arithmetic.As<bool>();
		}
		else if (arithmetic.HoldsFloat()) {
			json_node[key] = arithmetic.As<long double>();
		}
		else if (arithmetic.HoldsSigned()) {
			json_node[key] = arithmetic.As<std::ptrdiff_t>();
		}
		else if (arithmetic.HoldsSigned()) {
			json_node[key] = arithmetic.As<std::size_t>();
		}
		else {

		}

	}

	void JSONConfig::ProceedConfigArray(ConfigNode::Value const& val, nlohmann::json& json_node, std::string const& key) {

		ConfigNode::Value::Array const& array = val.AsArray();
		nlohmann::json json_array = nlohmann::json::array();

		for (auto const& element : array) {

			if (element.HoldsArithmetic()){

				ConfigNode::Value::Arithmetic const& arithmetic = val.AsArithmetic();

				if (arithmetic.HoldsBool()) {
					json_array.push_back(arithmetic.As<bool>());
				}
				else if (arithmetic.HoldsFloat()) {
					json_array.push_back(arithmetic.As<long double>());
				}
				else if (arithmetic.HoldsSigned()) {
					json_array.push_back(arithmetic.As<std::ptrdiff_t>());
				}
				else if (arithmetic.HoldsSigned()) {
					json_array.push_back(arithmetic.As<std::size_t>());
				}
				else {

				}

			}
			else if (element.HoldsString()) {
				std::string const& str = element.AsString();
				json_array.push_back(str);
			}
			else {

			}

		}

		json_node[key] = json_array;

	}

	void JSONConfig::SerializeConfig(ConfigNode const& root, nlohmann::json& json_root) {

		struct FrameStack {
			ConfigNode::ConstIterator begin;
			ConfigNode::ConstIterator end;
			nlohmann::json* json_node;
		};

		std::array<std::byte, 8192u> buffer{};
		std::pmr::monotonic_buffer_resource pool(
			buffer.data(),
			buffer.size(),
			std::pmr::null_memory_resource()
		);

		std::pmr::vector<FrameStack> stack(&pool);
		stack.push_back({ root.begin(), root.end(), &json_root });

		while (!stack.empty()) {

			auto [current_begin, current_end, json_node] = std::move(stack.back());
			stack.pop_back();

			for (auto iter = current_begin; iter != current_end; ++iter) {

				auto const& [key, val] = *iter;

				switch (val.GetStorageType()) {
				case ConfigNode::Value::StorageType::Arithmetic:
					(*json_node)[key] = val.AsOr(0.0f);
					break;

				case ConfigNode::Value::StorageType::String:
					(*json_node)[key] = val.AsOr(std::string());
					break;

				case ConfigNode::Value::StorageType::Array:
					JSONConfig::ProceedConfigArray(val, *json_node, key);
					break;

				case ConfigNode::Value::StorageType::Node:
					JSONConfig::ProceedConfigNode(val, *json_node, key, stack);
					break;
				}
			}

		}

	}

	void JSONConfig::OpenImpl(std::filesystem::path const& file_path) {

		if (!std::filesystem::exists(file_path)) {
			throw std::runtime_error("path does not exists");
		}

		std::ifstream stream(file_path);

		nlohmann::json data = nlohmann::json::parse(stream);
		JSONConfig::ParseJSON(data, m_root);

	}

	void JSONConfig::SaveAsImpl(std::filesystem::path const& file_path) const {

		auto parent_path = file_path.parent_path();
		if (!parent_path.empty() && !std::filesystem::exists(parent_path)) {
			// Create parent directories if they don't exist
			std::filesystem::create_directories(parent_path);
		}

		std::ofstream out(file_path);
		nlohmann::json json_node = nlohmann::json::object();

		JSONConfig::SerializeConfig(m_root, json_node);

		out << json_node;


	}

	std::string JSONConfig::DumpImpl() const {

		nlohmann::json json_node = nlohmann::json::object();

		JSONConfig::SerializeConfig(m_root, json_node);

		return json_node.dump(4);

	}

	void JSONConfig::ParseImpl(std::string_view dumped) {
		nlohmann::json data = nlohmann::json::parse(dumped);
		JSONConfig::ParseJSON(data, m_root);
	}

	JSONConfig::JSONConfig(std::filesystem::path const& file_path) {
		OpenImpl(file_path);
	}

}