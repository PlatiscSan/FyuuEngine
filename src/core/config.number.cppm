export module config:number;
import std;

namespace core {
	class Number {
	public:
		using NumberVariant = std::variant<
			std::monostate,
			std::uint8_t,
			std::uint16_t,
			std::uint32_t,
			std::uint64_t,
			std::int8_t,
			std::int16_t,
			std::int32_t,
			std::int64_t,
			bool,
			float,
			double
		>;

	private:
		NumberVariant m_num;

	public:
		Number(std::monostate);

		template <class Value>
			requires std::is_arithmetic_v<std::decay_t<Value>>
		Number(Value&& value)
			: m_num(std::forward<Value>(value)) {

		}

		template <class Value>
			requires std::is_arithmetic_v<std::decay_t<Value>>
		Number& operator=(Value&& value) {
			m_num.emplace<std::decay_t<Value>>(std::forward<Value>(value));
			return *this;
		}

		template <class Value>
		std::optional<Value> Get() const noexcept {
			return std::visit(
				[](auto&& num)
				-> std::optional<Value> {
					using Storage = std::decay_t<decltype(num)>;
					if constexpr (std::is_arithmetic_v<Storage> && std::is_convertible_v<Storage, Value>) {
						return static_cast<Value>(num);
					}
					else {
						return std::nullopt;
					}
				},
				m_num
			);
		}

		NumberVariant& Variant() noexcept;
		NumberVariant const& Variant() const noexcept;
		bool IsNumber() const noexcept;
		operator bool() const noexcept;

	};

	bool IsFloatingPoint(std::string_view str);
	Number ConvertToNumber(std::string_view str);

}