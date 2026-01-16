export module plastic.arithmetic;
import std;

namespace plastic::utility {

	export class Arithmetic {
	private:
		using ValueType = std::variant<
			std::monostate,
			bool,
			std::uint8_t,
			std::int8_t,
			std::uint16_t,
			std::int16_t,
			std::uint32_t,
			std::int32_t,
			std::uint64_t,
			std::int64_t,
			float,
			double,
			long double
		>;

		ValueType m_value;

	public:
		Arithmetic() = default;

		template <class T>
		Arithmetic(T&& value) 
			: m_value(std::in_place_type<std::decay_t<T>>, std::forward<T>(value)) {
			static_assert(std::is_arithmetic_v<std::decay_t<T>>, "Arithmetic only supports arithmetic types");
		}

		Arithmetic(Arithmetic const& other) = default;

		Arithmetic(Arithmetic&& other) = default;

		std::string_view TypeName() const noexcept;

		std::string ToString() const;

		template <class T>
		bool HoldsType() const noexcept {
			using Type = std::decay_t<T>;
			static_assert(std::is_arithmetic_v<Type>, "T must be an arithmetic type");
			return std::holds_alternative<Type>(m_value);
		}

		bool HoldsBool() const noexcept;
		bool HoldsFloat() const noexcept;
		bool HoldsUnsigned() const noexcept;
		bool HoldsSigned() const noexcept;

		template <class T>
		T As() const noexcept {
			using Type = std::decay_t<T>;
			static_assert(std::is_arithmetic_v<std::decay_t<T>>, "T must be an arithmetic type");
			return std::visit(
				[](auto&& arg) -> Type {
					using U = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<U, std::monostate>) {
						return static_cast<Type>(0u);
					}
					else if constexpr (std::is_same_v<U, Type>) {
						return arg;
					}
					else {
						return static_cast<Type>(arg);
					}
				},
				m_value
			);
		}

		Arithmetic operator+(Arithmetic const& other) const;
		Arithmetic operator-(Arithmetic const& other) const;
		Arithmetic operator*(Arithmetic const& other) const;
		Arithmetic operator/(Arithmetic const& other) const;
		bool operator==(Arithmetic const& other) const;
		bool operator!=(Arithmetic const& other) const;

		friend std::ostream& operator<<(std::ostream& os, Arithmetic const& arith);

	};

}