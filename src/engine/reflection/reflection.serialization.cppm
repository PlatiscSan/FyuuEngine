export module reflection:serialization;
export import config.yaml;
export import config.json;
import std;

namespace fyuu_engine::reflection {

	export struct YAMLSerializer {
		std::string serialized;

		void operator()(auto&& key, auto&& value) {
			std::string key_ = key;
			config::YAMLConfig result;
			result[key_].Set(value);
			serialized = result.ToString();
		}
	};

	export struct YAMLDeserializer {
		std::string_view serialized;

		void operator()(auto&& key, auto&& value) {
			//std::string key_ = key;
			//value = node[key_].Get<std::decay_t<decltype(value)>>();
		}
	};

	export struct JSONSerializer {
	private:
		std::variant<std::monostate, config::JSONConfig, std::string> m_result;

	public:
		void operator()(auto&& key, auto&& value) {

			std::string key_ = key;

			std::visit(
				[this, &key_, &value](auto&& result) {
					using Type = std::decay_t<decltype(result)>;

					if constexpr (std::is_same_v<Type, config::JSONConfig>) {
						result[key_].Set(value);
					}
					else {
						auto& new_result = m_result.emplace<config::JSONConfig>();
						new_result[key_].Set(value);
					}

				},
				m_result
			);
		}

		std::string_view Serialized();

	};

}