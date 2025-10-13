module config:number;

template <class Value>
std::optional<Value> TryConvertion(std::string_view str) {

	if constexpr (std::is_same_v<Value, bool>) {
		// Handle bool conversion
		if (str == "true" || str == "1") {
			return true;
		}
		else if (str == "false" || str == "0") {
			return false;
		}
		else {
			return std::nullopt;
		}
	}
	else {
		// For arithmetic types that are not bool
		Value val;
		auto result = std::from_chars(
			str.data(), str.data() + str.size(), val
		);
		if (result.ec == std::errc() &&
			result.ptr == str.data() + str.size()) {
			return val;
		}
		return std::nullopt;
	}

}

namespace fyuu_engine::core {

	Number::Number(std::monostate)
		: m_num(std::monostate()) {

	}
	Number::NumberVariant& Number::Variant() noexcept {
		return m_num;
	}

	Number::NumberVariant const& Number::Variant() const noexcept {
		return m_num;
	}

	bool Number::IsNumber() const noexcept {
		return m_num.index() == 0;
	}

	Number::operator bool() const noexcept {
		return !IsNumber();
	}

	bool IsFloatingPoint(std::string_view str) {

		bool has_decimal = false;
		bool has_exponent = false;
		bool digit_before_exp = false;
		bool digit_after_exp = false;

		size_t start = 0;

		if (str[0] == '-' || str[0] == '+') {
			start = 1;
		}

		for (size_t i = start; i < str.size(); ++i) {

			char const c = str[i];

			if (c == '.') {

				if (has_decimal || has_exponent) {
					// Multiple decimal points or decimal points after index
					return false;
				}
				has_decimal = true;
			}
			else if (c == 'e' || c == 'E') {
				if (has_exponent || !digit_before_exp) {
					// Multiple indexes or no number before indexes
					return false;
				}
				has_exponent = true;
				if (i + 1 < str.size() && (str[i + 1] == '+' || str[i + 1] == '-')) {
					++i;
				}
			}
			else if (std::isdigit(static_cast<unsigned char>(c))) {
				if (has_exponent) {
					digit_after_exp = true;
				}
				else {
					digit_before_exp = true;
				}
			}
			else {
				return false;
			}
		}

		// Verify that there are numbers in the index part
		if (has_exponent && !digit_after_exp) {
			return false;
		}
		return has_decimal || has_exponent || !digit_before_exp;
	}

	Number ConvertToNumber(std::string_view str) {

		if (str.empty()) {
			return std::monostate();
		}

		// Try integer conversion
		if (!IsFloatingPoint(str)) {
			if (auto val = TryConvertion<bool>(str)) {
				return (*val);
			}

			if (auto val = TryConvertion<std::uint8_t>(str)) {
				return *val;
			}

			if (auto val = TryConvertion<std::int8_t>(str)) {
				return *val;
			}

			if (auto val = TryConvertion<std::uint16_t>(str)) {
				return *val;
			}

			if (auto val = TryConvertion<std::int16_t>(str)) {
				return *val;
			}

			if (auto val = TryConvertion<std::uint32_t>(str)) {
				return *val;
			}

			if (auto val = TryConvertion<std::int32_t>(str)) {
				return *val;
			}

			if (auto val = TryConvertion<std::uint64_t>(str)) {
				return *val;
			}

			if (auto val = TryConvertion<std::int64_t>(str)) {
				return *val;
			}

			return std::monostate();

		}

		// Try floating point conversion

		float flt_val;
		auto flt_result = std::from_chars(
			str.data(), str.data() + str.size(), flt_val
		);

		if (flt_result.ec == std::errc() &&
			flt_result.ptr == str.data() + str.size()) {
			return flt_val;
		}

		double dbl_val;
		auto dbl_result = std::from_chars(
			str.data(), str.data() + str.size(), dbl_val
		);

		if (dbl_result.ec == std::errc() &&
			dbl_result.ptr == str.data() + str.size()) {
			return dbl_val;
		}

		return std::monostate();

	}

}