export module reflection:serialization;
export import config.yaml;
export import config.json;
import std;
import :interface;

namespace fyuu_engine::reflection {

	namespace details {

		template <class Serializer, class Field>
		concept HasMakeReflective = requires (Serializer serializer, Field field) {
			requires ReflectiveType<decltype(Serializer::MakeReflective(field))>;
		};

	}

	export template <class Derived> class BaseJSONSerializer {
	private:
		std::variant<std::monostate, config::JSONConfig, std::string> m_buffer;

		/*
		*	field serialization and deserialization tag dispatching
		*/

		/// @brief for custom field serialization
		/// @tparam Accessor field accessor
		/// @tparam Depth recursion depth
		/// @param node config node
		/// @param field_accessor field accessor
		template <std::size_t Depth, FieldAccessorConcept Accessor>
		static void SerializeField(config::ConfigNode& node, Accessor&& field_accessor) {

			static_assert(Depth < 4, "deep recursion is banned");

			if constexpr (std::bool_constant<std::decay_t<Accessor>::is_pointer>::value) {
				
				/*
				*	pointer branch
				*/

				if (auto ptr = field_accessor.Get()) {

					static_assert(details::HasMakeReflective<Derived, std::remove_pointer_t<std::decay_t<decltype(std::declval<Accessor>().Get())>>>,
						"No proper reflective");

					auto reflective = Derived::MakeReflective(*ptr);

					if constexpr (std::bool_constant<Depth == 0>::value) {

						/*
						*	first call, the node is passed by 
						*	void SerializeField(config::JSONConfig& config, Accessor&& field_accessor, std::true_type)
						*	OR
						*	void SerializeField(config::JSONConfig& config, Accessor&& field_accessor, std::false_type)
						*/

						reflective.Visit(
							[&node](auto&& field_accessor) {
								SerializeField<Depth + 1>(node, field_accessor);
							}
						);
					}
					else {

						/*
						*	recursive call
						*/

						std::string key = field_accessor.Name();
						config::ConfigNode& next = node[key].AsNode();
						reflective.Visit(
							[&next](auto&& field_accessor) {
								SerializeField<Depth + 1>(next, field_accessor);
							}
						);
					}
				}
			}
			else {

				if constexpr (requires{node[std::declval<std::string>()].Set(field_accessor.Get()); }) {
					/*
					*	basic type branch and the first call ends here if not custom field type.
					*/

					std::string key = field_accessor.Name();
					node[key].Set(field_accessor.Get());
				}
				else {

					/*
					*	custom type field branch then call recursively
					*/

					static_assert(details::HasMakeReflective<Derived, std::remove_pointer_t<std::decay_t<decltype(std::declval<Accessor>().Get())>>>,
						"No proper reflective");

					auto value = field_accessor.Get();
					auto reflective = Derived::MakeReflective(value);
					reflective.Visit(
						[&node](auto&& field_accessor) {
							SerializeField<Depth + 1>(node, field_accessor);
						}
					);

				}
			}

		}

		template <std::size_t Depth, FieldAccessorConcept Accessor>
		static void DeserializeField(config::ConfigNode const& node, Accessor&& field_accessor) {

			static_assert(Depth < 4, "deep recursion is banned");
			using FieldType = std::decay_t<decltype(field_accessor.Get())>;

			if constexpr (std::bool_constant<std::decay_t<Accessor>::is_pointer>::value) {

				static_assert(details::HasMakeReflective<Derived, std::remove_pointer_t<std::decay_t<decltype(std::declval<Accessor>().Get())>>>,
					"No proper reflective");

				decltype(auto) ptr = field_accessor.Get();

				if constexpr (std::bool_constant<Depth == 0>::value) {

					/*
					*	first call, the node is passed by
					*	void DeserializeField(config::JSONConfig& config, Accessor&& field_accessor, std::true_type)
					*	OR
					*	void DeserializeField(config::JSONConfig& config, Accessor&& field_accessor, std::false_type)
					*/

					auto reflective = Derived::MakeReflective(*ptr);

					reflective.Visit(
						[&node](auto&& field_accessor) {
							DeserializeField<Depth + 1>(node, field_accessor);
						}
					);

				}
				else {

					/*
					*	create pointer field if it is null 
					*/

					using Pointer = std::decay_t<decltype(field_accessor.Get())>;
					using ActualType = typename std::decay_t<Accessor>::ActualType;

					decltype(auto) ptr = field_accessor.Get();
					if (!ptr) {
						if constexpr (std::is_convertible_v<std::shared_ptr<ActualType>, Pointer>) {
							ptr = std::make_shared<ActualType>();
							field_accessor.Set(ptr);
						}
						if constexpr (std::is_convertible_v<std::unique_ptr<ActualType>, Pointer>) {
							ptr = std::make_unique<ActualType>();
						}
						else {
							static_assert(std::is_convertible_v<std::add_pointer_t<ActualType>, Pointer>,
								"Some fields are not deserializable");
							ptr = new ActualType();
							field_accessor.Set(ptr);
						}
					}

					try {

						auto reflective = Derived::MakeReflective(*ptr);
						std::string key = field_accessor.Name();

						/*
						*	field value might not exist
						*/

						auto const& next = node[key].Get<config::ConfigNode>();

						/*
						*	recursive call
						*/
						reflective.Visit(
							[&next](auto&& field_accessor) {
								DeserializeField<Depth + 1>(next, field_accessor);
							}
						);
					}
					catch (std::out_of_range const&) {
						/*
						*	field value does not exist, stop recursion
						*/
						return;
					}

				}

			}
			else {

				constexpr bool is_ordinary_field =
					requires{ { field_accessor.Set(0) } -> std::same_as<void>; } ||
					requires{ { field_accessor.Set(0.0f) } -> std::same_as<void>; } ||
					requires{ { field_accessor.Set(false) } -> std::same_as<void>; } ||
					requires{ { field_accessor.Set(std::declval<std::string>()) } -> std::same_as<void>; };

				if constexpr (std::bool_constant<is_ordinary_field>::value) {
					/*
					*	basic type branch and the first call ends here if not custom field type.
					*/

					std::string key = field_accessor.Name();

					if constexpr (std::is_arithmetic_v<FieldType>) {
						field_accessor.Set(node[key].GetOr(0.0f));
					}
					else if constexpr (std::is_convertible_v<std::string, FieldType>) {
						field_accessor.Set(node[key].GetOr(std::string()));
					}
					else {

					}
				}
				else {

					static_assert(details::HasMakeReflective<Derived, std::remove_pointer_t<std::decay_t<decltype(std::declval<Accessor>().Get())>>>,
						"No proper reflective");

					decltype(auto) value = field_accessor.Get();
					auto reflective = Derived::MakeReflective(value);
					reflective.Visit(
						[&node](auto&& field_accessor) {
							DeserializeField<Depth + 1>(node, field_accessor);
						}
					);

				}

			}
		}

		/// @brief	for pointer field serialization
		/// @tparam Serializer 
		/// @tparam Accessor 
		/// @param serializer 
		/// @param field_accessor 
		/// @param  
		template <FieldAccessorConcept Accessor>
		static void SerializeField(config::JSONConfig& config, Accessor&& field_accessor, std::true_type) {
			if (auto ptr = field_accessor.Get()) {
				std::string key = field_accessor.Name();
				config::ConfigNode& next = config[key].AsNode();
				BaseJSONSerializer::SerializeField<0>(next, field_accessor);
			}
		}

		/// @brief	for ordinary field serialization
		/// @tparam Serializer 
		/// @tparam Accessor 
		/// @param serializer 
		/// @param field_accessor 
		/// @param  
		template <FieldAccessorConcept Accessor>
		static void SerializeField(config::JSONConfig& config, Accessor&& field_accessor, std::false_type) {
			std::string key = field_accessor.Name();
			if constexpr (requires{config[key].Set(field_accessor.Get()); }) {
				config[key].Set(field_accessor.Get());
			}
			else {
				/*
				*	custom field type branch.
				*/
				config::ConfigNode& next = config[key].AsNode();
				SerializeField<0>(next, field_accessor);
			}
		}


		/// @brief	for pointer field deserialization
		/// @tparam Serializer 
		/// @tparam Accessor 
		/// @param serializer 
		/// @param field_accessor 
		/// @param  
		template <FieldAccessorConcept Accessor>
		static void DeserializeField(config::JSONConfig const& config, Accessor&& field_accessor, std::true_type) {

			/*
			*	create pointer field if it is null
			*/

			using Pointer = std::decay_t<decltype(field_accessor.Get())>;
			using ActualType = typename std::decay_t<Accessor>::ActualType;

			decltype(auto) ptr = field_accessor.Get();
			if (!ptr) {
				if constexpr (std::is_convertible_v<std::shared_ptr<ActualType>, Pointer>) {
					ptr = std::make_shared<ActualType>();
					field_accessor.Set(ptr);
				}
				if constexpr (std::is_convertible_v<std::unique_ptr<ActualType>, Pointer>) {
					ptr = std::make_unique<ActualType>();
				}
				else {
					static_assert(std::is_convertible_v<std::add_pointer_t<ActualType>, Pointer>,
						"Some fields are not deserializable");
					ptr = new ActualType();
					field_accessor.Set(ptr);
				}
			}

			try {
				std::string key = field_accessor.Name();

				/*
				*	field value might not exist
				*/

				auto& node = config[key].Get<config::ConfigNode>();

				DeserializeField<0>(node, field_accessor);
			}
			catch (std::out_of_range const&) {
				/*
				*	field value does not exist, return
				*/
				return;
			}


		}

		/// @brief	for ordinary field deserialization
		/// @tparam Serializer 
		/// @tparam Accessor 
		/// @param serializer 
		/// @param field_accessor 
		/// @param  
		template <FieldAccessorConcept Accessor>
		static void DeserializeField(config::JSONConfig const& config, Accessor&& field_accessor, std::false_type) {
			
			constexpr bool is_ordinary_field =
				requires{ { field_accessor.Set(0) } -> std::same_as<void>; } ||
				requires{ { field_accessor.Set(0.0f) } -> std::same_as<void>; } ||
				requires{ { field_accessor.Set(false) } -> std::same_as<void>; } ||
				requires{ { field_accessor.Set(std::declval<std::string>()) } -> std::same_as<void>; };

			std::string key = field_accessor.Name();
			auto const& value = config[key];

			if constexpr (std::bool_constant<is_ordinary_field>::value) {

				using FieldType = std::decay_t<decltype(field_accessor.Get())>;

				if constexpr (std::is_arithmetic_v<FieldType>) {
					field_accessor.Set(value.GetOr(0.0f));
				}
				else if constexpr (std::is_convertible_v<std::string, FieldType>) {
					field_accessor.Set(value.GetOr(std::string()));
				}
				else {

				}

			}
			else {
				/*
				*	custom field type branch.
				*/

				try {
					std::string key = field_accessor.Name();

					/*
					*	field value might not exist
					*/

					auto& node = config[key].Get<config::ConfigNode>();

					DeserializeField<0>(node, field_accessor);
				}
				catch (std::out_of_range const&) {
					/*
					*	field value does not exist, return
					*/
					return;
				}

			}
		}

	public:
		using Field = config::ConfigNode;

		void Parse(std::string_view serialized) {
			config::JSONConfig& config = m_buffer.emplace<config::JSONConfig>();
			config.Parse(serialized);
		}

		void BeginSerialization() {
			m_buffer.emplace<config::JSONConfig>();
		}

		void EndSerialization() {
			std::visit(
				[this](auto&& buffer) {
					using Buffer = std::decay_t<decltype(buffer)>;
					if constexpr (std::is_same_v<Buffer, config::JSONConfig>) {
						config::JSONConfig& config = buffer;
						std::string serialized = config.Dump();
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

		template <FieldAccessorConcept Accessor>
		void Serialize(Accessor&& field_accessor) {
			
			std::visit(
				[this, &field_accessor](auto&& buffer) {

					using Buffer = std::decay_t<decltype(buffer)>;

					if constexpr (std::is_same_v<Buffer, config::JSONConfig>) {				
						BaseJSONSerializer::SerializeField(buffer, field_accessor, std::bool_constant<std::decay_t<Accessor>::is_pointer>());
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

		template <FieldAccessorConcept Accessor>
		void Deserialize(Accessor&& field_accessor) {

			std::visit(
				[this, &field_accessor](auto&& buffer) {
					using Type = std::decay_t<decltype(buffer)>;

					if constexpr (std::is_same_v<Type, config::JSONConfig>) {					
						BaseJSONSerializer::DeserializeField(buffer, field_accessor, std::bool_constant<std::decay_t<Accessor>::is_pointer>());
					}
					else {
						/*
						*	Parse() must be called or buffer has the valid config before deserialization, or everything is ignored
						*/

					}

				},
				m_buffer
			);
		}

	};

	export class JSONSerializer : public BaseJSONSerializer<JSONSerializer> {

	};

}