export module serialization;
import config;
import reflective;
import std;

namespace fyuu_engine::reflection {

	namespace details {

		template <class Serializer, class Field>
		concept HasMakeReflective = requires (Serializer serializer, Field field) {
			requires ReflectiveConcept<decltype(Serializer::MakeReflective(field))>;
		};

		template <class Serializer, class Field>
		concept HasSerialize = requires (Serializer serializer, Field const& field, config::NodeValue node_val) {
			{ Serializer::Serialize(field, node_val) } -> std::same_as<void>;
		};

		template <class Serializer, class Field>
		concept HasDeserialize = requires (Serializer serializer, Field field, config::NodeValue const& node_val) {
			{ Serializer::Deserialize(field, node_val) } -> std::same_as<void>;
		};

		template <FieldAccessorConcept Accessor>
		using FieldValueType = std::remove_pointer_t<std::decay_t<decltype(std::declval<Accessor>().Get())>>;

		template <class T>
		struct IsSTDVectorImpl {
			static constexpr bool value = false;
		};

		template <class T, class Alloc>
		struct IsSTDVectorImpl<std::vector<T, Alloc>> {
			static constexpr bool value = true;
		};

		template <class T>
		using IsSTDVector = std::integral_constant<bool, IsSTDVectorImpl<std::remove_cv_t<std::remove_reference_t<T>>>::value>;

		template <class T>
		inline constexpr bool IsSTDVectorValue = IsSTDVector<T>::value;

		template <class T>
		concept STDVector = IsSTDVectorValue<T>;

		template <class T>
		struct ArrayDeserializationStrategy {
			template <STDVector Vector>
			void Push(Vector&&, T&) const {

			}
		};

		template <>
		struct ArrayDeserializationStrategy<config::Arithmetic> {
			template <STDVector Vector>
			void Push(Vector&& vector, config::Arithmetic& arithmetic) const {
				if (arithmetic.HoldsBool()) {
					vector.push_back(arithmetic.As<bool>());
				}
				else if (arithmetic.HoldsFloat()) {
					vector.push_back(arithmetic.As<long double>());
				}
				else if (arithmetic.HoldsSigned()) {
					vector.push_back(arithmetic.As<std::ptrdiff_t>());
				}
				else if (arithmetic.HoldsUnsigned) {
					vector.push_back(arithmetic.As<std::size_t>());
				}
				else {

				}
			}
		};

		template <>
		struct ArrayDeserializationStrategy<std::string> {
			template <STDVector Vector>
			void Push(Vector&& vector, std::string& str) const {
				vector.push_back(str);
			}
		};

		template <class FieldType>
		struct OrdinaryFieldDeserializationStrategy {
			template <FieldAccessorConcept Accessor>
			void Set(Accessor&&, config::NodeValue const&) {

			}
		};

		template <class T>
		concept Arithmetic = std::is_arithmetic_v<T>;

		template <Arithmetic FieldType>
		struct OrdinaryFieldDeserializationStrategy<FieldType> {
			template <FieldAccessorConcept Accessor>
			void Set(Accessor&& field_accessor, config::NodeValue const& value) {
				field_accessor.Set(value.AsOr(0.0f));
			}
		};


		template <>
		struct OrdinaryFieldDeserializationStrategy<std::string> {
			template <FieldAccessorConcept Accessor>
			void Set(Accessor&& field_accessor, config::NodeValue const& value) {
				field_accessor.Set(value.AsOr(std::string()));
			}
		};

		template <STDVector FieldType>
		struct OrdinaryFieldDeserializationStrategy<FieldType> {
			template <FieldAccessorConcept Accessor>
			void Set(Accessor&& field_accessor, config::NodeValue const& value) {

				FieldType vector{};
				vector.reserve(16u);

				auto const& array = value.As<config::NodeValue::Array>();
				using ValueType = typename FieldType::value_type;

				for (auto const& element : array) {
					std::visit(
						[&vector](auto&& element) {
							using Element = std::decay_t<decltype(element)>;
							details::ArrayDeserializationStrategy<Element>().Push(vector, element);
						},
						element
					);
				}

			}
		};

		template <class Serializer, std::size_t Depth, FieldAccessorConcept Accessor>
		struct CustomDeserializationStrategy {

			static constexpr bool is_valid = false;

			void operator()(config::ConfigNode const&, Accessor&&) {
			
			}

		};

		template <class Serializer, std::size_t Depth, FieldAccessorConcept Accessor>
		struct MakeReflectiveDeserialization
			: CustomDeserializationStrategy<Serializer, Depth, Accessor> {

			static constexpr bool is_valid = true;

			void operator()(config::ConfigNode const& node, Accessor&& field_accessor) {
				using FieldType = typename std::decay_t<Accessor>::FieldType;
				auto value = field_accessor.Get();
				auto reflective = Serializer::MakeReflective(value);
				reflective.Visit(
					[&node](auto&& nested_field) {
						Serializer::DeserializeField<Depth + 1>(node, nested_field);
					}
				);
			}

		};

		template <class Serializer, std::size_t Depth, FieldAccessorConcept Accessor>
		struct CustomDeserialization
			: CustomDeserializationStrategy<Serializer, Depth, Accessor> {

			static constexpr bool is_valid = true;

			void operator()(config::ConfigNode const& node, Accessor&& field_accessor) {
				using FieldType = typename std::decay_t<Accessor>::FieldType;
				using ActualType = typename std::decay_t<Accessor>::ActualType;
				ActualType instance{};
				config::NodeValue const& value = node[field_accessor.Name()];
				Serializer::Deserialize(instance, value);
				field_accessor.Set(std::move(instance));
			}

		};

		template <class Serializer, std::size_t Depth, FieldAccessorConcept Accessor>
		struct CustomDeserializationStrategySelector {
			using Type = CustomDeserializationStrategy<Serializer, Depth, Accessor>;
		};

		template <class Serializer, std::size_t Depth, FieldAccessorConcept Accessor>
			requires (HasMakeReflective<Serializer, details::FieldValueType<Accessor>> && !HasDeserialize<Serializer, details::FieldValueType<Accessor>>)
		struct CustomDeserializationStrategySelector<Serializer, Depth, Accessor> {
			using Type = MakeReflectiveDeserialization<Serializer, Depth, Accessor>;
		};

		template <class Serializer, std::size_t Depth, FieldAccessorConcept Accessor>
			requires (HasDeserialize<Serializer, details::FieldValueType<Accessor>> && !HasMakeReflective<Serializer, details::FieldValueType<Accessor>>)
		struct CustomDeserializationStrategySelector<Serializer, Depth, Accessor> {
			using Type = CustomDeserialization<Serializer, Depth, Accessor>;
		};

		template <class Serializer, std::size_t Depth, FieldAccessorConcept Accessor>
			requires (HasMakeReflective<Serializer, details::FieldValueType<Accessor>> && HasDeserialize<Serializer, details::FieldValueType<Accessor>>)
		struct CustomDeserializationStrategySelector<Serializer, Depth, Accessor> {
			using Type = CustomDeserialization<Serializer, Depth, Accessor>;
		};

		template <class Serializer, std::size_t Depth, FieldAccessorConcept Accessor>
		struct DeserializeCustomType
			: CustomDeserializationStrategySelector<Serializer, Depth, Accessor>::Type {

			using Base = typename CustomDeserializationStrategySelector<Serializer, Depth, Accessor>::Type;
			using Base::Base;
			using Base::operator();
		};

	}

	/// @brief	Class template that implements JSON serialization and deserialization logic for reflective types. 
	///			It provides field-level (de)serialization with special handling for pointer fields, ordinary/basic fields, STL-like vectors, and nested/custom reflective types. 
	///			Recursion depth is limited to prevent deep recursion. 
	///			The implementation uses config::JSONConfig and config::ConfigNode and relies on a Derived type to provide reflection via MakeReflective.
	/// @tparam Derived The derived serializer type that must provide a static MakeReflective(...) facility and satisfy the reflective requirements used by this base. 
	///					Derived is used to obtain a reflective view of target objects so the base can visit and (de)serialize their fields.
	export template <class Derived> class BaseJSONSerializer {
		template <class Serializer, std::size_t Depth, FieldAccessorConcept Accessor>
		friend struct details::MakeReflectiveDeserialization;
	private:
		std::variant<std::monostate, config::JSONConfig, std::string> m_buffer;

		template <std::size_t Depth, FieldAccessorConcept Accessor>
		struct SerializeCustomType {
			static constexpr bool is_valid = false;
		};

		template <std::size_t Depth, FieldAccessorConcept Accessor>
			requires details::HasMakeReflective<Derived, details::FieldValueType<Accessor>>
		struct SerializeCustomType<Depth, Accessor> {
			void operator()(config::ConfigNode& node, Accessor&& field_accessor) {
				using FieldType = typename std::decay_t<Accessor>::FieldType;
				auto& value_node = node[field_accessor.Name()].AsNode();
				auto reflective = Derived::MakeReflective(field_accessor.Get());
				reflective.Visit(
					[&value_node](auto&& nested_field) {
						SerializeField<Depth + 1>(value_node, nested_field);
					}
				);
			}
		};

		/*template <std::size_t Depth, FieldAccessorConcept Accessor>
		static void SerializeCustomType(config::ConfigNode& node, Accessor&& field_accessor) {
			using FieldType = typename std::decay_t<Accessor>::FieldType;

			if constexpr (details::HasCustomSerialization<FieldType>) {
				std::string key = field_accessor.Name();
				auto& value_node = node[key].AsNode();
				details::CustomSerializer<FieldType>::Serialize(value_node, field_accessor.Get());
			}
			else {
				auto reflective = Derived::MakeReflective(field_accessor.Get());
				reflective.Visit(
					[&node](auto&& nested_field) {
						SerializeField<Depth + 1>(node, nested_field);
					}
				);
			}
		}*/

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

					static_assert(details::HasMakeReflective<Derived, details::FieldValueType<Accessor>>,
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

				//if constexpr (requires{node[std::declval<std::string>()].Set(field_accessor.Get()); }) {
				//	/*
				//	*	basic type branch and the first call ends here if not custom field type.
				//	*/

				//	std::string key = field_accessor.Name();
				//	node[key].Set(field_accessor.Get());
				//}
				//else {

				//	/*
				//	*	custom type field branch then call recursively
				//	*/

				//	static_assert(details::HasMakeReflective<Derived, std::remove_pointer_t<std::decay_t<decltype(std::declval<Accessor>().Get())>>>,
				//		"No proper reflective");

				//	auto value = field_accessor.Get();
				//	auto reflective = Derived::MakeReflective(value);
				//	reflective.Visit(
				//		[&node](auto&& field_accessor) {
				//			SerializeField<Depth + 1>(node, field_accessor);
				//		}
				//	);

				//}

				constexpr bool is_basic_type =
					requires { node[std::declval<std::string>()].Set(field_accessor.Get()); } || details::STDVector<typename std::decay_t<Accessor>::FieldType>;

				if constexpr (std::bool_constant<is_basic_type>::value) {
					std::string key = field_accessor.Name();
					node[key].Set(field_accessor.Get());
				}
				else {
					static_assert(SerializeCustomType<Depth, Accessor>::is_valid,
						"No proper reflective or custom type serializer");
					SerializeCustomType<Depth, Accessor>()(node, field_accessor);
				}
				//else if constexpr (details::HasCustomSerialization<typename std::decay_t<Accessor>::FieldType>) {
				//	SerializeCustomType<Depth>(node, field_accessor);
				//}
				//else {
				//	auto value = field_accessor.Get();
				//	auto reflective = Derived::MakeReflective(value);
				//	reflective.Visit(
				//		[&node](auto&& nested_field) {
				//			SerializeField<Depth + 1>(node, nested_field);
				//		}
				//	);
				//}

			}

		}

		//template <std::size_t Depth, FieldAccessorConcept Accessor, class = void>
		//struct DeserializeCustomType {
		//	static constexpr bool is_valid = false;
		//};

		//template <std::size_t Depth, FieldAccessorConcept Accessor>
		//	requires details::HasMakeReflective<Derived, details::FieldValueType<Accessor>>
		//struct DeserializeCustomType<Depth, Accessor> {
		//	static constexpr bool is_valid = true;

		//	void operator()(config::ConfigNode const& node, Accessor&& field_accessor) {
		//		using FieldType = typename std::decay_t<Accessor>::FieldType;
		//		auto value = field_accessor.Get();
		//		auto reflective = Derived::MakeReflective(value);
		//		reflective.Visit(
		//			[&node](auto&& nested_field) {
		//				DeserializeField<Depth + 1>(node, nested_field);
		//			}
		//		);
		//	}
		//};


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
						*	std::out_of_range is thrown when field value does not exist
						*/

						auto const& next = node[key].As<config::ConfigNode>();

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
					requires{ { field_accessor.Set(std::declval<std::string>()) } -> std::same_as<void>; } ||
					details::STDVector<FieldType>;

				//if constexpr (std::bool_constant<is_ordinary_field>::value) {
				//	/*
				//	*	basic type branch and the first call ends here if not custom field type.
				//	*/

				//	std::string key = field_accessor.Name();
				//	details::OrdinaryFieldDeserializationStrategy<FieldType>().Set(field_accessor, node[key]);

				//}
				//else {

				//	static_assert(details::HasMakeReflective<Derived, std::remove_pointer_t<std::decay_t<decltype(std::declval<Accessor>().Get())>>>,
				//		"No proper reflective");

				//	decltype(auto) value = field_accessor.Get();
				//	auto reflective = Derived::MakeReflective(value);

				//	/*
				//	*	recursive call
				//	*/
				//	reflective.Visit(
				//		[&node](auto&& field_accessor) {
				//			DeserializeField<Depth + 1>(node, field_accessor);
				//		}
				//	);

				//}

				if constexpr (std::bool_constant<is_ordinary_field>::value) {
					std::string key = field_accessor.Name();
					details::OrdinaryFieldDeserializationStrategy<FieldType>().Set(field_accessor, node[key]);
				}
				else {
					using DeserializeCustomType = details::DeserializeCustomType<Derived, Depth, Accessor>;

					static_assert(DeserializeCustomType::is_valid,
						"No proper reflective or custom deserialization");
					DeserializeCustomType()(node, field_accessor);
				}
				//else {
				//	auto value = field_accessor.Get();
				//	auto reflective = Derived::MakeReflective(value);
				//	reflective.Visit(
				//		[&node](auto&& nested_field) {
				//			DeserializeField<Depth + 1>(node, nested_field);
				//		}
				//	);
				//}

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
				*	std::out_of_range is thrown when field value does not exist
				*/

				auto& node = config[key].As<config::ConfigNode>();

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
			
			using FieldType = std::decay_t<decltype(field_accessor.Get())>;

			constexpr bool is_ordinary_field =
				requires{ { field_accessor.Set(0) } -> std::same_as<void>; } ||
				requires{ { field_accessor.Set(0.0f) } -> std::same_as<void>; } ||
				requires{ { field_accessor.Set(false) } -> std::same_as<void>; } ||
				requires{ { field_accessor.Set(std::declval<std::string>()) } -> std::same_as<void>; } ||
				details::STDVector<FieldType>;

			std::string key = field_accessor.Name();
			fyuu_engine::config::NodeValue const& value = config[key];

			if constexpr (std::bool_constant<is_ordinary_field>::value) {
				details::OrdinaryFieldDeserializationStrategy<FieldType>().Set(field_accessor, value);
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

					auto& node = config[key].As<config::ConfigNode>();

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

	/// @brief the serializer interface concept
	export template <class Serializer> concept SerializerConcept = requires(Serializer serializer) {
		{ serializer.Parse(std::declval<std::string_view>()) } -> std::same_as<void>;
		{ serializer.BeginSerialization() } -> std::same_as<void>;
		{ serializer.EndSerialization() } -> std::same_as<void>;
		{ serializer.Serialized() } -> std::convertible_to<std::string_view>;
	};

	/// @brief	A JSON serializer class that uses the Curiously Recurring Template Pattern (CRTP) to inherit JSON serialization behavior from BaseJSONSerializer<JSONSerializer>.
	///			If any custom field type needs special handling, please derive from BaseJSONSerializer and implement the custom logic there.
	///			This serializer only provides the default JSON serialization and deserialization logic for reflective types.
	export class JSONSerializer : public BaseJSONSerializer<JSONSerializer> {

	};

}