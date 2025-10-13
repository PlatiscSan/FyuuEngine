module;
#include <yaml-cpp/yaml.h>

module config:yaml;
import std;
import :node;
import defer;

static auto Lock(std::atomic_flag& mutex) {
	while (mutex.test_and_set(std::memory_order::acq_rel)) {
		mutex.wait(true, std::memory_order::relaxed);
	}
	return fyuu_engine::util::Defer(
		[&mutex]() {
			mutex.clear(std::memory_order::release);
			mutex.notify_one();
		}
	);
}

namespace fyuu_engine::core {

	YAMLConfig::YAMLConfig(YAMLConfig&& other) noexcept
		: m_node(std::move(other.m_node)),
		m_current_config(std::move(other.m_current_config)),
		m_last_modified(std::move(other.m_last_modified)) {

	}

	YAMLConfig& YAMLConfig::operator=(YAMLConfig&& other) noexcept {
		
		if (this != &other) {
			
			auto lock = Lock(m_mutex);
			auto other_lock = Lock(other.m_mutex);

			m_node = std::move(other.m_node);
			m_current_config = std::move(other.m_current_config);
			m_last_modified = std::move(other.m_last_modified);

		}

		return *this;

	}

	ConfigNode& core::YAMLConfig::Node() noexcept {
		return m_node;
	}

	ConfigNode::ConfigValue YAMLConfig::ParseYAMLNode(YAML::Node const& yaml_node) {

		if (!yaml_node || yaml_node.IsNull()) {
			return std::monostate();
		}

		switch (yaml_node.Type()) {
		case YAML::NodeType::Scalar:
		{
			std::string scalar = yaml_node.Scalar();
			Number scalar_num = ConvertToNumber(scalar);
			if (scalar_num) {
				return scalar_num;
			}
			return scalar;
		}

		case YAML::NodeType::Sequence:
		{
			bool all_scalar = true;
			for (auto const& node : yaml_node) {
				if (!node.IsScalar()) {
					all_scalar = false;
					break;
				}
			}

			if (all_scalar) {

				std::vector<Number> numbers;
				bool all_number = true;
				for (auto const& node : yaml_node) {
					std::string scalar = node.Scalar();
					Number num = ConvertToNumber(scalar);
					if (!num) {
						all_number = false;
						break;
					}
					numbers.push_back(std::move(num));
				}

				if (all_number) {
					return numbers;
				}


				std::vector<std::string> strings;
				for (auto const& node : yaml_node) {
					strings.push_back(node.Scalar());
				}

				return strings;

			}

			return std::monostate();
		}

		case YAML::NodeType::Map:
		{
			typename ConfigNode::Map temp_map;
			for (auto it = yaml_node.begin(); it != yaml_node.end(); ++it) {
				temp_map.try_emplace(it->first.Scalar(), ParseYAMLNode(it->second));
			}
			return temp_map;
		}

		default:
			return std::monostate();
		}
	}

	YAML::Node YAMLConfig::SerializeNumber(Number const& num) {
		return std::visit(
			[](auto&& arg) -> YAML::Node {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, std::monostate>) {
					return YAML::Node(YAML::NodeType::Null);
				}
				else if constexpr (std::is_same_v<T, bool>) {
					return YAML::Node(arg);
				}
				else if constexpr (std::is_integral_v<T>) {
					return YAML::Node(static_cast<int64_t>(arg));
				}
				else if constexpr (std::is_floating_point_v<T>) {
					return YAML::Node(arg);
				}
				else {
					return YAML::Node(YAML::NodeType::Null);
				}
			},
			num.Variant()
		);
	}

	YAML::Node YAMLConfig::SerializeConfigValue(ConfigNode::ConfigValue const& value) {
		return std::visit(
			[](auto&& arg) -> YAML::Node {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, std::monostate>) {
					return YAML::Node(YAML::NodeType::Null);
				}
				else if constexpr (std::is_same_v<T, Number>) {
					return YAML::Node(arg.Get<std::size_t>().value_or(0));
				}
				else if constexpr (std::is_same_v<T, std::string>) {
					return YAML::Node(arg);
				}
				else if constexpr (std::is_same_v<T, std::vector<Number>>) {
					YAML::Node node(YAML::NodeType::Sequence);
					for (auto const& num : arg) {
						node.push_back(SerializeNumber(num));
					}
					return node;
				}
				else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
					YAML::Node node(YAML::NodeType::Sequence);
					for (auto const& str : arg) {
						node.push_back(str);
					}
					return node;
				}
				else if constexpr (std::is_same_v<T, ConfigNode>) {
					YAML::Node node(YAML::NodeType::Map);
					for (auto const& [key, val] : arg.Root()) {
						node[key] = SerializeConfigValue(val);
					}
					return node;
				}
				else {

				}
			},
			value
		);
	}

	bool YAMLConfig::Open(std::filesystem::path const& file_path) {
		if (!std::filesystem::exists(file_path)) {
			return false;
		}

		auto lock = Lock(m_mutex);

		try {
			YAML::Node root = YAML::LoadFile(file_path.string());
			if (!root.IsMap()) {
				return false;
			}

			auto root_value = ParseYAMLNode(root);
			if (!std::holds_alternative<ConfigNode>(root_value)) {
				return false;
			}

			m_node = std::get<ConfigNode>(std::move(root_value));
			m_current_config = file_path;
			m_last_modified = std::filesystem::last_write_time(file_path);
			return true;
		}
		catch (YAML::Exception const&) {
			return false;
		}
	}

	bool YAMLConfig::Reopen() {
		if (m_current_config.empty()) {
			return false;
		}
		return Open(m_current_config);
	}

	bool YAMLConfig::Save() {
		if (m_current_config.empty()) {
			return false;
		}
		return SaveAs(m_current_config);
	}

	bool YAMLConfig::SaveAs(std::filesystem::path const& file_path) {

		/*
		*	Lock the node
		*/

		auto lock = Lock(m_mutex);

		try {
			YAML::Node root = SerializeConfigValue(m_node);
			std::ofstream fout(file_path);
			if (!fout) {
				return false;
			}
			fout << root;
			fout.close();

			m_current_config = file_path;
			m_last_modified = std::filesystem::last_write_time(file_path);
			return true;
		}
		catch (...) {
			return false;
		}
	}

}