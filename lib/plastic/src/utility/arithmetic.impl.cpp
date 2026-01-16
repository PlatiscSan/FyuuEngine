module plastic.arithmetic;

namespace plastic::utility {

	std::string_view Arithmetic::TypeName() const noexcept {
		return std::visit(
			[](auto&& arg) -> std::string {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, bool>) return "bool";
				else if constexpr (std::is_same_v<T, std::int8_t>) return "std::int8_t";
				else if constexpr (std::is_same_v<T, std::uint8_t>) return "std::uint8_t";
				else if constexpr (std::is_same_v<T, std::int16_t>) return "std::int16_t";
				else if constexpr (std::is_same_v<T, std::uint16_t>) return "std::uint16_t";
				else if constexpr (std::is_same_v<T, std::int32_t>) return "std::int32_t";
				else if constexpr (std::is_same_v<T, std::uint32_t>) return "std::uint32_t";
				else if constexpr (std::is_same_v<T, std::int64_t>) return "std::int64_t";
				else if constexpr (std::is_same_v<T, std::uint64_t>) return "std::uint64_t";
				else if constexpr (std::is_same_v<T, float>) return "float";
				else if constexpr (std::is_same_v<T, double>) return "double";
				else if constexpr (std::is_same_v<T, long double>) return "long double";
				else return "invalid";
			},
			m_value
		);
	}

	std::string Arithmetic::ToString() const {
		return std::visit(
			[](auto&& arg) -> std::string {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, std::monostate>) {
					return "invalid";
				}
				else if constexpr (std::is_same_v<T, bool>) {
					return arg ? "true" : "false";
				}
				else {
					return std::to_string(arg);
				}
			},
			m_value
		);
	}

	bool Arithmetic::HoldsBool() const noexcept {
		return HoldsType<bool>();
	}

	bool Arithmetic::HoldsFloat() const noexcept {
		return 
			HoldsType<float>() || 
			HoldsType<double>() || 
			HoldsType<long double>();
	}

	bool Arithmetic::HoldsUnsigned() const noexcept {
		return 
			HoldsType<std::uint8_t>() || 
			HoldsType<std::uint16_t>() || 
			HoldsType<std::uint32_t>() || 
			HoldsType<std::uint64_t>();
	}

	bool Arithmetic::HoldsSigned() const noexcept {
		return 		
			HoldsType<std::int8_t>() ||
			HoldsType<std::int16_t>() ||
			HoldsType<std::int32_t>() ||
			HoldsType<std::int64_t>();;
	}

	Arithmetic Arithmetic::operator+(Arithmetic const& other) const {
		return std::visit(
			[this](auto&& arg1, auto&& arg2) -> Arithmetic {
				
				using T1 = std::decay_t<decltype(arg1)>;
				using T2 = std::decay_t<decltype(arg2)>;

				if constexpr (std::is_same_v<T1, std::monostate> || std::is_same_v<T2, std::monostate>) {
					throw std::runtime_error("invalid arithmetic type");
				}
				else if constexpr (std::is_floating_point_v<T1> || std::is_floating_point_v<T2>) {
					return static_cast<double>(arg1) + static_cast<double>(arg2);
				}
				else {
					return arg1 + arg2;
				}

			},
			m_value,
			other.m_value
		);
	}

	Arithmetic Arithmetic::operator-(Arithmetic const& other) const {
		return std::visit(
			[this](auto&& arg1, auto&& arg2) -> Arithmetic {

				using T1 = std::decay_t<decltype(arg1)>;
				using T2 = std::decay_t<decltype(arg2)>;

				if constexpr (std::is_same_v<T1, std::monostate> || std::is_same_v<T2, std::monostate>) {
					throw std::runtime_error("invalid arithmetic type");
				}
				else if constexpr (std::is_floating_point_v<T1> || std::is_floating_point_v<T2>) {
					return static_cast<double>(arg1) - static_cast<double>(arg2);
				}
				else {
					return arg1 - arg2;
				}

			},
			m_value,
			other.m_value
		);
	}

	Arithmetic Arithmetic::operator*(Arithmetic const& other) const {
		return std::visit(
			[this](auto&& arg1, auto&& arg2) -> Arithmetic {

				using T1 = std::decay_t<decltype(arg1)>;
				using T2 = std::decay_t<decltype(arg2)>;

				if constexpr (std::is_same_v<T1, std::monostate> || std::is_same_v<T2, std::monostate>) {
					throw std::runtime_error("invalid arithmetic type");
				}
				else if constexpr (std::is_floating_point_v<T1> || std::is_floating_point_v<T2>) {
					return static_cast<double>(arg1) * static_cast<double>(arg2);
				}
				else {
					return arg1 * arg2;
				}
			},
			m_value,
			other.m_value
		);
	}

	Arithmetic Arithmetic::operator/(Arithmetic const& other) const {
		return std::visit(
			[this](auto&& arg1, auto&& arg2) -> Arithmetic {

				using T1 = std::decay_t<decltype(arg1)>;
				using T2 = std::decay_t<decltype(arg2)>;

				if constexpr (std::is_same_v<T1, std::monostate> || std::is_same_v<T2, std::monostate>) {
					throw std::runtime_error("invalid arithmetic type");
				}
				else if constexpr (std::is_floating_point_v<T1> || std::is_floating_point_v<T2>) {
					return static_cast<double>(arg1) / static_cast<double>(arg2);
				}
				else {
					return arg1 / arg2;
				}
			},
			m_value,
			other.m_value
		);
	}

	bool Arithmetic::operator==(Arithmetic const& other) const {
		return std::visit(
			[this](auto&& arg1, auto&& arg2) -> bool {

				using T1 = std::decay_t<decltype(arg1)>;
				using T2 = std::decay_t<decltype(arg2)>;

				if constexpr (std::is_same_v<T1, std::monostate> || std::is_same_v<T2, std::monostate>) {
					throw std::runtime_error("invalid arithmetic type");
				}
				else if constexpr (std::is_floating_point_v<T1> || std::is_floating_point_v<T2>) {
					return std::abs(arg1 - arg2) < 1e-5;
				}
				else {
					return arg1 == arg2;
				}
			},
			m_value,
			other.m_value
		);
	}

	bool Arithmetic::operator!=(Arithmetic const& other) const {
		return !(*this == other);;
	}

	std::ostream& operator<<(std::ostream& os, Arithmetic const& arith) {
		std::visit(
			[&os](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, std::monostate>) {
					os << "NaN";
				}
				else {
					os << arg;
				}
			},
			arith.m_value
		);
		return os;
	}

}