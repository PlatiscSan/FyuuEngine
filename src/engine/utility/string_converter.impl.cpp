module string_converter;

namespace fyuu_engine::util {

	bool StringConverter::IsBoolString(std::string const& str) {
		static const char* true_values[] = { "true", "false", "1", "0", "yes", "no", "off", "on" };
		std::string lower = ToLower(str);

		for (const char* val : true_values) {
			if (lower == val) return true;
		}
		return false;
	}

	std::string StringConverter::ToLower(std::string const& str) {
		std::string result = str;
		std::transform(
			result.begin(), 
			result.end(), 
			result.begin(),
			[](unsigned char c) {
				return std::tolower(c); 
			}
		);
		return result;
	}

	bool StringConverter::ConvertToBool(std::string const& str) {
		std::string lower = ToLower(str);
		return (lower == "true" || lower == "1" || lower == "yes");
	}

	std::int64_t StringConverter::ConvertToInt(std::string const& str) {
		return std::stoll(str);
	}

	std::uint64_t StringConverter::ConvertToUint(std::string const& str) {
		return std::stoull(str);
	}

	double StringConverter::ConvertToFloat(std::string const& str) {
		return std::stof(str);
	}

	StringConverter::ValueType StringConverter::InferType(std::string const& str) {

		if (str.empty()) {
			return ValueType::String;
		}

		if (IsBoolString(str)) {
			return ValueType::Bool;
		}

		bool has_dot = false;
		bool has_exponent = false;
		bool has_digits = false;
		bool has_sign = (str[0] == '+' || str[0] == '-');

		std::size_t start = has_sign ? 1 : 0;

		for (std::size_t i = start; i < str.length(); ++i) {
			char c = str[i];

			if (c == '.') {
				if (has_dot || has_exponent) {
					// has more than one dot or dot after exponent
					return ValueType::String;
				}
				has_dot = true;
			}
			else if (c == 'e' || c == 'E') {
				if (has_exponent || !has_digits) {
					// has more than one exponent or exponent without preceding digits
					return ValueType::String;
				}
				has_exponent = true;
				// Checks if there is a sign after the exponent
				if (i + 1 < str.length() && (str[i + 1] == '+' || str[i + 1] == '-')) {
					// skip the sign
					++i;
				}

				bool exponent_has_digits = false;
				// Check for digits after exponent
				for (std::size_t j = i + 1; j < str.length(); ++j) {
					if (std::isdigit(static_cast<unsigned char>(str[j]))) {
						exponent_has_digits = true;
						break;
					}
				}

				if (!exponent_has_digits) {
					// no digits after exponent
					return ValueType::String;
				}

			}
			else if (!std::isdigit(static_cast<unsigned char>(c))) {
				// invalid character found
				return ValueType::String;
			}
		}

		if (has_dot || has_exponent) {
			// it's a float
			return ValueType::Float;
		}

		if (!has_sign && str.length() > 1 && str[0] == '0') {
			// leading zero in a non-negative number, treat as unsigned
			return ValueType::Uint;
		}

		if (has_sign && str[0] == '-') {

			return ValueType::Int;
		}

		// default to uint

		return ValueType::Uint;


	}

}