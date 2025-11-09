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
				if constexpr (std::is_same_v<Number, Type>) {
					return ConfigNode::Value::StorageType::Number;
				}
				else if constexpr (std::is_same_v<std::string, Type>) {
					return ConfigNode::Value::StorageType::String;
				}
				else if constexpr (std::is_same_v<Array, Type>) {
					return ConfigNode::Value::StorageType::Array;
				}
				else if constexpr (std::is_same_v<std::shared_ptr<ConfigNode>, Type>) {
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

	void ConfigNode::Value::Set(std::string_view str) {
		m_storage.emplace<std::string>(str);
	}

	void ConfigNode::Value::Set(Number const& num) {
		m_storage.emplace<Number>(num);
	}

	void ConfigNode::Value::Set(std::shared_ptr<ConfigNode> const& node) {
		m_storage.emplace<std::shared_ptr<ConfigNode>>(node);
	}

	void ConfigNode::Value::AsNode() {
		Set(std::make_shared<ConfigNode>());
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
				if constexpr (std::is_same_v<std::shared_ptr<ConfigNode>, std::decay_t<decltype(storage)>>) {
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
				if constexpr (std::is_same_v<std::shared_ptr<ConfigNode>, std::decay_t<decltype(storage)>>) {
					return (*storage)[path];
				}
				else {
					throw std::runtime_error("not node type");
				}
			},
			m_storage
		);
	}

	ConfigNode::Value::ArrayElement& ConfigNode::Value::operator[](std::size_t index) {
		return std::visit(
			[index](auto&& storage) -> ConfigNode::Value::ArrayElement& {
				if constexpr (std::is_same_v<Array, std::decay_t<decltype(storage)>>) {
					if (index >= storage.size()) {
						throw std::out_of_range("array index out of range");
					}
					return storage[index];
				}
				else {
					throw std::runtime_error("not an array type");
				}
			},
			m_storage
		);
	}


	ConfigNode::Value  const& ConfigNode::Value::operator[](std::string const& path) const {
		return std::visit(
			[&path](auto&& storage) -> Value const& {
				if constexpr (std::is_same_v<std::shared_ptr<ConfigNode>, std::decay_t<decltype(storage)>>) {
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
				if constexpr (std::is_same_v<std::shared_ptr<ConfigNode>, std::decay_t<decltype(storage)>>) {
					return (*storage)[path];
				}
				else {
					throw std::runtime_error("not node type");
				}
			},
			m_storage
		);
	}

	ConfigNode::Value::ArrayElement const& ConfigNode::Value::operator[](std::size_t index) const {
		return std::visit(
			[index](auto&& storage) -> ConfigNode::Value::ArrayElement const& {
				if constexpr (std::is_same_v<Array, std::decay_t<decltype(storage)>>) {
					if (index >= storage.size()) {
						throw std::out_of_range("array index out of range");
					}
					return storage[index];
				}
				else {
					throw std::runtime_error("not an array type");
				}
			},
			m_storage
		);
	}

}