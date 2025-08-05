export module config:node;
import :number;
import std;

namespace core {
	export class ConfigNode {
	public:
		using ConfigValue = std::variant<
			std::monostate,
			Number,
			std::vector<Number>,
			std::string,
			std::vector<std::string>,
			ConfigNode
		>;
		using Map = std::unordered_map<std::string, ConfigValue>;

	private:
		Map m_root;

		static std::vector<std::string> SplitSegment(std::string_view segment);

		std::pair<Map*, std::string> TraverseForWrite(std::vector<std::string> const& segments);

		std::pair<Map const*, std::string> TraverseForRead(std::vector<std::string> const& segments) const;

	public:
		ConfigNode() = default;

		template <std::convertible_to<Map> CompatibleMap>
		ConfigNode(CompatibleMap&& map)
			: m_root(std::forward<CompatibleMap>(map)) {

		}

		Map const& Root() const noexcept;

		template <std::convertible_to<ConfigNode> Node>
		void Root(Node&& node) noexcept {
			m_root = std::forward<Node>(node);
		}

		template <std::convertible_to<std::string> Key>
		ConfigValue& operator[](Key&& key) {
			auto segments = SplitSegment(std::forward<Key>(key));
			if (segments.empty()) {
				throw std::invalid_argument("Empty key");
			}
			auto [last_map, last_key] = TraverseForWrite(segments);
			return last_map[last_key];
		}

		template <std::convertible_to<std::string> Key, class Value>
		void Set(Key&& key, Value&& value) {
			auto segments = SplitSegment(std::forward<Key>(key));
			if (segments.empty()) {
				throw std::invalid_argument("Empty key");
			}
			auto [last_map, last_key] = TraverseForWrite(segments);
			last_map->try_emplace(last_key, std::forward<Value>(value));
		}

		template <class Value, std::convertible_to<std::string> Key>
		std::optional<Value> Get(Key&& key) const {
			auto segments = SplitSegment(std::forward<Key>(key));
			if (segments.empty()) {
				return std::nullopt;
			}

			auto [last_map, last_key] = TraverseForRead(segments);
			if (!last_map) {
				return std::nullopt;
			}

			auto it = last_map->find(last_key);
			if (it == last_map->end()) {
				return std::nullopt;
			}

			return std::visit(
				[](auto&& value) -> std::optional<Value> {
					using Storage = std::decay_t<decltype(value)>;
					if constexpr (std::is_same_v<Storage, Number> && std::is_convertible_v<Storage, Value>) {
						return value.Get<Value>();
					}
					else if constexpr (std::is_same_v<Storage, std::vector<Number>> && std::is_convertible_v<std::vector<Number>, Value>) {
						return static_cast<Value>(value);
					}
					else if constexpr (std::is_same_v<Storage, std::string> && std::is_convertible_v<std::string, Value>) {
						return static_cast<Value>(value);
					}
					else if constexpr (std::is_same_v<Storage, std::vector<std::string>> && std::is_convertible_v<std::vector<std::string>, Value>) {
						return static_cast<Value>(value);
					}
					else {
						return std::nullopt;
					}
				},
				it->second
			);

		}

	};
}