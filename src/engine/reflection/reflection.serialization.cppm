export module reflection:serialization;
export import config.yaml;
export import config.json;
import std;

namespace fyuu_engine::reflection {

	//export struct YAMLSerializer {
	//	std::string serialized;

	//	void operator()(auto&& key, auto&& value) {
	//		std::string key_ = key;
	//		config::YAMLConfig result;
	//		result[key_].Set(value);
	//		serialized = result.ToString();
	//	}
	//};

	//export class JSONSerializer {
	//private:
	//	std::variant<std::monostate, config::JSONConfig, std::string> m_result;

	//public:
	//	void Serialize(auto&& key, auto&& value) {

	//		std::string key_ = key;

	//		std::visit(
	//			[this, &key_, &value](auto&& result) {
	//				using Type = std::decay_t<decltype(result)>;

	//				if constexpr (std::is_same_v<Type, config::JSONConfig>) {
	//					result[key_].Set(value);
	//				}
	//				else {
	//					auto& new_result = m_result.emplace<config::JSONConfig>();
	//					new_result[key_].Set(value);
	//				}

	//			},
	//			m_result
	//		);
	//	}

	//	std::string_view Serialized();

	//};

	//export template <class Derived> class CustomJSONSerializer : public JSONSerializer {
	//private:
	//	std::string_view m_field_name;
	//	bool m_pointer_field;

	//public:
	//	void PointerFieldStarts(std::string_view field_name) {
	//		m_field_name = field_name;
	//		m_pointer_field = true;
	//	}

	//	void PointerFieldEnds() {
	//		m_pointer_field = false;
	//	}


	//};

	namespace details {

		template <class T, class Key, class Value, class = void>
		struct HasSerializeImpl : std::false_type {};

		template <class T, class Key, class Value>
		struct HasSerializeImpl<T, Key, Value, std::void_t<decltype(std::declval<T>().SerializeImpl(std::declval<Key>(), std::declval<Value>()))>>
			: std::true_type {
		};

	}

	export template <class Derived> class BaseJSONSerializer {
	private:
		std::variant<std::monostate, config::JSONConfig, std::string> m_buffer;

	protected:
		config::JSONConfig& Config() {
			return std::get<config::JSONConfig>(m_buffer);
		}

	public:
		void BeginSerialization() {
			m_buffer.emplace<config::JSONConfig>();
		}

		void EndSerialization() {
			std::visit(
				[this](auto&& buffer) {
					using Buffer = std::decay_t<decltype(buffer)>;
					if constexpr (std::is_same_v<Buffer, config::JSONConfig>) {
						config::JSONConfig& config = buffer;
						std::string serialized = config.ToString();
						m_buffer.emplace<std::string>(std::move(serialized));
					}
					else {
						/*
						*	BeginSerialization() must be called before EndSerialization(), or everything is ignored
						*/

					}
				},
				m_buffer
			);
		}

		void Serialize(auto&& key, auto&& value) {

			std::string key_ = key;

			std::visit(
				[this, &key, &value](auto&& buffer) {
					using Type = std::decay_t<decltype(buffer)>;

					if constexpr (std::is_same_v<Type, config::JSONConfig>) {

						/// config::JSONConfig can only accessed by std:string
						std::string key_ = key;

						if constexpr (requires{buffer[key_].Set(value); }) {
							/*
							*	No need for custom serialization if the value is string or arithmetic type.
							*/
							buffer[key_].Set(value);
						}
						else {
							/*
							*	pointer field or other custom class and structure will branch here.
							*/
							static_assert(details::HasSerializeImpl<Derived, std::decay_t<decltype(key)>, std::decay_t<decltype(value)>>::value,
								"a proper override SerializeImpl function is required");
							static_cast<Derived*>(this)->SerializeImpl(key, value);
						}
					}
					else {
						/*
						*	BeginSerialization() must be called before serialization, or everything is ignored
						*/

					}

				},
				m_buffer
			);
		}

		std::string_view Serialized() const noexcept {
			return std::visit(
				[this](auto&& buffer) -> std::string_view {
					using Buffer = std::decay_t<decltype(buffer)>;
					if constexpr (std::is_same_v<Buffer, std::string>) {
						return buffer;
					}
					else {
						return "";
					}
				},
				m_buffer
			);
		}

	};

	export class JSONSerializer : public BaseJSONSerializer<JSONSerializer> {

	};

}