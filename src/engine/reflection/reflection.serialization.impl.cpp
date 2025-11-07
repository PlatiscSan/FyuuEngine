module reflection:serialization;

namespace fyuu_engine::reflection {

	std::string_view JSONSerializer::Serialized() {
		return std::visit(
			[this](auto&& result) -> std::string_view {
				using Type = std::decay_t<decltype(result)>;

				if constexpr (std::is_same_v<Type, config::JSONConfig>) {
					std::string serialized = result.ToString();
					return m_result.emplace<std::string>(std::move(serialized));
				}
				else if constexpr (std::is_same_v<Type, std::string>) {
					return result;
				}
				else {
					return "";
				}

			},
			m_result
		);
	}
}