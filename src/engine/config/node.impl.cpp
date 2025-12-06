module config:node;

namespace fyuu_engine::config {

	ConfigNode::Value& ConfigNode::operator[](std::string const& key) {
		return m_fields[key];
	}

	ConfigNode::Value const& ConfigNode::operator[](std::string const& key) const {
		return m_fields.at(key);
	}

	ConfigNode::Iterator ConfigNode::begin() noexcept {
		return m_fields.begin();
	}

	ConfigNode::Iterator ConfigNode::end() noexcept {
		return m_fields.end();
	}

	ConfigNode::ConstIterator ConfigNode::begin() const noexcept {
		return m_fields.begin();
	}

	ConfigNode::ConstIterator ConfigNode::end() const noexcept {
		return m_fields.end();
	}

	bool ConfigNode::Contains(std::string const& field) const noexcept {
		return m_fields.contains(field);
	}

	ConfigNode::Value::operator bool() const noexcept {
		return std::visit(
			[](auto&& storage) {
				if constexpr (std::is_same_v<std::monostate, std::decay_t<decltype(storage)>>) {
					return false;
				}
				else {
					return true;
				}
			},
			m_storage
		);
	}

	ConfigNode::Value::StorageType ConfigNode::Value::GetStorageType() const noexcept {
		return std::visit(
			[](auto&& storage) -> ConfigNode::Value::StorageType {
				using Type = std::decay_t<decltype(storage)>;
				if constexpr (std::is_same_v<Arithmetic, Type>) {
					return ConfigNode::Value::StorageType::Arithmetic;
				}
				else if constexpr (std::is_same_v<std::string, Type>) {
					return ConfigNode::Value::StorageType::String;
				}
				else if constexpr (std::is_same_v<Array, Type>) {
					return ConfigNode::Value::StorageType::Array;
				}
				else if constexpr (std::is_same_v<std::unique_ptr<ConfigNode>, Type>) {
					return ConfigNode::Value::StorageType::Node;
				}
				else {
					return ConfigNode::Value::StorageType::Null;
				}
			},
			m_storage
		);
	}

	void ConfigNode::Value::Set() {
		m_storage.emplace<std::monostate>();
	}

	ConfigNode& ConfigNode::Value::AsNode() {
		std::unique_ptr<ConfigNode>& storage = std::get<std::unique_ptr<ConfigNode>>(m_storage);
		return *storage;
	}

	ConfigNode const& ConfigNode::Value::AsNode() const {
		std::unique_ptr<ConfigNode> const& storage = std::get<std::unique_ptr<ConfigNode>>(m_storage);
		return *storage;
	}

	ConfigNode::Value::Array& ConfigNode::Value::AsArray() {
		return std::get<Array>(m_storage);
	}

	ConfigNode::Value::Array const& ConfigNode::Value::AsArray() const {
		return std::get<Array>(m_storage);
	}

	ConfigNode::Value::Arithmetic& ConfigNode::Value::AsArithmetic() {
		return std::get<Arithmetic>(m_storage);
	}

	ConfigNode::Value::Arithmetic const& ConfigNode::Value::AsArithmetic() const {
		return std::get<Arithmetic>(m_storage);
	}

	void ConfigNode::Value::Set(Array const& array) {
		m_storage.emplace<Array>(array);
	}

	void ConfigNode::Value::Set(Array&& array) {
		m_storage.emplace<Array>(std::move(array));
	}

	ConfigNode::Value& ConfigNode::Value::operator[](std::string const& path) {
		return std::visit(
			[&path](auto&& storage) -> Value& {
				if constexpr (std::is_same_v<std::unique_ptr<ConfigNode>, std::decay_t<decltype(storage)>>) {
					return (*storage)[path];
				}
				else {
					throw std::runtime_error("not node type");
				}
			},
			m_storage
		);
	}

	ConfigNode::Value& ConfigNode::Value::operator[](char const* path) {
		return std::visit(
			[path](auto&& storage) -> Value& {
				if constexpr (std::is_same_v<std::unique_ptr<ConfigNode>, std::decay_t<decltype(storage)>>) {
					return (*storage)[path];
				}
				else {
					throw std::runtime_error("not node type");
				}
			},
			m_storage
		);
	}

	ConfigNode::Value const& ConfigNode::Value::operator[](std::string const& path) const {
		return std::visit(
			[&path](auto&& storage) -> Value const& {
				if constexpr (std::is_same_v<std::unique_ptr<ConfigNode>, std::decay_t<decltype(storage)>>) {
					return (*storage)[path];
				}
				else {
					throw std::runtime_error("not node type");
				}
			},
			m_storage
		);
	}

	ConfigNode::Value const& ConfigNode::Value::operator[](char const* path) const {
		return std::visit(
			[path](auto&& storage) -> Value const& {
				if constexpr (std::is_same_v<std::unique_ptr<ConfigNode>, std::decay_t<decltype(storage)>>) {
					return (*storage)[path];
				}
				else {
					throw std::runtime_error("not node type");
				}
			},
			m_storage
		);
	}

}