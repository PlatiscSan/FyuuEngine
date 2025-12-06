module array;

namespace fyuu_engine::util {
	
	ArrayElement::ValueType ArrayElement::ConstructValueFrom(ArrayElement const& other) {
		return std::visit(
			[](auto&& value) -> ValueType {
				
				using Value = std::decay_t<decltype(value)>;

				if constexpr (std::is_same_v<Value, Arithmetic>) {
					return ValueType(std::in_place_type<Arithmetic>, value);
				}
				else if constexpr (std::is_same_v<Value, std::string>) {
					return ValueType(std::in_place_type<std::string>, value);
				}
				else if constexpr (std::is_same_v<std::unique_ptr<Array>, std::string>) {
					return ValueType(std::in_place_type<std::unique_ptr<Array>>, std::make_unique<Array>(*value));
				}
				else {
					return ValueType();
				}

			},
			other.m_value
		);
	}

	ArrayElement::ArrayElement(ArrayElement const& other)
		: m_value(ConstructValueFrom(other)) {
	}

	ArrayElement::ArrayElement(Arithmetic const& value)
		: m_value(std::in_place_type<Arithmetic>, value) {
	}

	ArrayElement::ArrayElement(Arithmetic&& value)
		: m_value(std::in_place_type<Arithmetic>, std::move(value)) {
	}

	ArrayElement::ArrayElement(std::string_view value)
		: m_value(std::in_place_type<std::string>, value) {
	}

	ArrayElement::ArrayElement(std::string&& value)
		: m_value(std::in_place_type<std::string>, std::move(value)) {
	}

	ArrayElement::ArrayElement(std::unique_ptr<Array>&& array)
		: m_value(std::in_place_type<std::unique_ptr<Array>>, std::move(array)) {
	}

	ArrayElement::ArrayElement(Array&& array)
		: m_value(std::in_place_type<std::unique_ptr<Array>>, std::make_unique<Array>(std::move(array))) {
	}

	bool ArrayElement::IsArithmetic() const noexcept {
		return std::holds_alternative<Arithmetic>(m_value);
	}

	bool ArrayElement::IsString() const noexcept {
		return std::holds_alternative<std::string>(m_value);
	}

	bool ArrayElement::IsArray() const noexcept {
		return std::holds_alternative<std::unique_ptr<Array>>(m_value);
	}

	bool ArrayElement::HoldsArithmetic() const noexcept {
		return std::holds_alternative<Arithmetic>(m_value);
	}

	bool ArrayElement::HoldsString() const noexcept {
		return std::holds_alternative<std::string>(m_value);
	}

	bool ArrayElement::HoldsArray() const noexcept {
		return std::holds_alternative<std::unique_ptr<Array>>(m_value);
	}

	Arithmetic& ArrayElement::AsArithmetic() {
		return std::get<Arithmetic>(m_value);
	}

	std::string& ArrayElement::AsString() {
		return std::get<std::string>(m_value);
	}

	Array& ArrayElement::AsArray() {
		std::unique_ptr<Array>& array = std::get<std::unique_ptr<Array>>(m_value);
		return *array;
	}

	Arithmetic const& ArrayElement::AsArithmetic() const {
		return std::get<Arithmetic>(m_value);
	}

	std::string const& ArrayElement::AsString() const {
		return std::get<std::string>(m_value);
	}

	Array const& ArrayElement::AsArray() const {
		std::unique_ptr<Array> const& array = std::get<std::unique_ptr<Array>>(m_value);
		return *array;
	}

	Array::iterator Array::begin() {
		return m_data.begin();
	}

	Array::iterator Array::end() {
		return m_data.end();
	}

	Array::const_iterator Array::begin() const {
		return m_data.begin();
	}

	Array::const_iterator Array::end() const {
		return m_data.end();
	}

	std::size_t fyuu_engine::util::Array::size() const {
		return m_data.size();
	}

	bool Array::empty() const {
		return m_data.empty();
	}

	void Array::reserve(std::size_t capacity) {
		m_data.reserve(capacity);
	}

	void Array::clear() {
		m_data.clear();
	}

	ArrayElement& Array::operator[](std::size_t index) {
		return m_data[index];
	}

	ArrayElement const& Array::operator[](std::size_t index) const {
		return m_data[index];
	}

}