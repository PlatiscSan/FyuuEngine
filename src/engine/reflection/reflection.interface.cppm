/*
* 
*	usage of the interface is referred to compilation_test.cpp
* 
*/
export module reflection:interface;
export import pointer_wrapper;
export import fixed_string;
import std;
import <nameof.hpp>;

namespace fyuu_engine::reflection {

	namespace details {

		struct Any {
			template <class T>
			operator T() const noexcept {
				return {};
			}
		};

		template <class T, class... Args> consteval std::size_t FieldCount() noexcept {

			static_assert(std::is_aggregate_v<T>, "FieldCount requires aggregate type");

			if constexpr (requires{T(Args()..., Any()); }) {
				return FieldCount<T, Args..., Any>();
			}
			else {
				return sizeof...(Args);
			}
		}

		template <class T>
		constexpr std::string_view TypeName() {
			return NAMEOF_TYPE(T);
		}

	}

	/*
	*	field pointer traits
	*/

	export template <class T> struct FieldPointerTraits {};

	export template <class Class, class Type> struct FieldPointerTraits<Type Class::*> {
		using ClassType = Class;
		using FieldType = Type;
		using ActualType = Type;
		static constexpr bool is_pointer = false;
		static constexpr bool is_smart_pointer = false;
	};

	/*
	*	for pointer fields
	*/

	/// @brief raw pointer trait
	/// @tparam Class class type
	/// @tparam Type field type
	export template <class Class, class Type> struct FieldPointerTraits<Type* Class::*> {
		using ClassType = Class;
		using FieldType = Type*;
		using ActualType = Type;
		static constexpr bool is_pointer = true;
		static constexpr bool is_smart_pointer = false;
	};

	/// @brief std::unique_ptr trait
	/// @tparam Class class type
	/// @tparam Type field type
	/// @tparam Deleter unique_ptr deleter
	export template <class Class, class Type, class Deleter> struct FieldPointerTraits<std::unique_ptr<Type, Deleter> Class::*> {
		using ClassType = Class;
		using FieldType = std::unique_ptr<Type, Deleter>;
		using ActualType = Type;
		static constexpr bool is_pointer = true;
		static constexpr bool is_smart_pointer = true;
	};

	/// @brief std::shared_ptr trait
	/// @tparam Class class type
	/// @tparam Type field type
	export template <class Class, class Type> struct FieldPointerTraits<std::shared_ptr<Type> Class::*> {
		using ClassType = Class;
		using FieldType = std::shared_ptr<Type>;
		using ActualType = Type;
		static constexpr bool is_pointer = true;
		static constexpr bool is_smart_pointer = true;
	};

	/// @brief util::PointerWrapper trait
	/// @tparam Class class type
	/// @tparam Type field type
	export template <class Class, class Type> struct FieldPointerTraits<util::PointerWrapper<Type> Class::*> {
		using ClassType = Class;
		using FieldType = util::PointerWrapper<Type>;
		using ActualType = Type;
		static constexpr bool is_pointer = true;
		static constexpr bool is_smart_pointer = true;
	};

	/*
	*	field descriptor
	*/

	/// @brief	This is used to declare your field in the associated Reflective type
	/// @tparam Name field name
	/// @tparam MemberPtr field pointer
	export template <util::FixedString Name, auto MemberPtr> struct FieldDescriptor {
		static_assert(std::is_member_pointer_v<decltype(MemberPtr)>,
			"FieldDescriptor requires a member pointer");

		static constexpr util::FixedString field_name = Name;
		static constexpr auto field_ptr = MemberPtr;

		using Traits = FieldPointerTraits<decltype(MemberPtr)>;
		using ClassType = typename Traits::ClassType;
		using FieldType = typename Traits::FieldType;

		static constexpr std::string_view type_name = details::TypeName<FieldType>();
	};

	export template <class T> concept IsFieldDescriptor = requires {
		typename T::ClassType;
		typename T::FieldType;
		requires requires { T::field_name; };
		requires requires { T::type_name; };
		requires requires { T::field_ptr; };
		requires std::is_member_pointer_v<decltype(T::field_ptr)>;
	};

	export template <class T, class... FieldDescriptors>
		concept ReflectiveConcept = requires {
		requires std::is_aggregate_v<T>;
		requires sizeof...(FieldDescriptors) == details::FieldCount<T>();
		requires (IsFieldDescriptor<FieldDescriptors> && ...);
		requires (std::is_same_v<typename FieldDescriptors::ClassType, T> && ...);
	};

	/*
	*	Reflective forward declaration
	*/

	export template <class T, class... FieldDescriptors>
		requires ReflectiveConcept<T, FieldDescriptors...>
	class Reflective;

	/// @brief 
	/// @tparam T 
	/// @tparam FieldDesc 
	export template <class T, class FieldDesc> class FieldAccessor {
	private:
		T* m_data;

	public:
		using FieldType = typename FieldDesc::FieldType;
		using ActualType = typename FieldDesc::Traits::ActualType;

		static constexpr bool is_pointer = FieldDesc::Traits::is_pointer;
		static constexpr bool is_smart_pointer = FieldDesc::Traits::is_smart_pointer;

		constexpr FieldAccessor(T* data) noexcept 
			: m_data(data) {}

		constexpr auto Get() const noexcept {
			return m_data->*(FieldDesc::field_ptr);
		}

		template <std::convertible_to<typename FieldDesc::FieldType> Value>
		constexpr void Set(Value&& value) noexcept {
			m_data->*(FieldDesc::field_ptr) = std::forward<Value>(value);
		}

		static constexpr auto Name() noexcept {
			return FieldDesc::field_name;
		}

		static constexpr std::string_view TypeName() noexcept {
			return FieldDesc::type_name;
		}

		static constexpr std::string_view ActualTypeName() noexcept {
			if constexpr (is_pointer) {
				return details::TypeName<ActualType>();
			}
			else {
				return TypeName();
			}
		}

	};

	export template <class T, class... FieldDescriptors>
		requires ReflectiveConcept<T, FieldDescriptors...>
	class Reflective {
	private:
		T* m_data;

		template <util::FixedString Name, std::size_t... Is>
		static constexpr std::size_t FindFieldIndexImpl(std::index_sequence<Is...>) noexcept {
			constexpr std::array<std::string_view, sizeof...(FieldDescriptors)> names = {
				FieldDescriptors::field_name...
			};

			std::size_t result = sizeof...(FieldDescriptors);
			((names[Is] == Name ? (result = Is, true) : false) || ...);
			return result;
		}

		template <util::FixedString Name>
		static constexpr std::size_t FindFieldIndex() noexcept {
			constexpr std::size_t index = FindFieldIndexImpl<Name>(
				std::index_sequence_for<FieldDescriptors...>{});

			static_assert(index < sizeof...(FieldDescriptors),
				"Field not found in reflective type");
			return index;
		}

		/*
		*	field serialization and deserialization tag dispatching
		*/

		/// @brief	for pointer field serialization
		/// @tparam Serializer 
		/// @tparam Accessor 
		/// @param serializer 
		/// @param field_accessor 
		/// @param  
		template <class Serializer, class Accessor>
		void SerializeField(Serializer&& serializer, Accessor&& field_accessor, std::true_type) const {
			if (auto ptr = field_accessor.Get()) {
				serializer.Serialize(field_accessor.Name(), *ptr);
			}
		}

		/// @brief	for ordinary field serialization
		/// @tparam Serializer 
		/// @tparam Accessor 
		/// @param serializer 
		/// @param field_accessor 
		/// @param  
		template <class Serializer, class Accessor>
		void SerializeField(Serializer&& serializer, Accessor&& field_accessor, std::false_type) const {
			serializer.Serialize(field_accessor.Name(), field_accessor.Get());
		}


		/// @brief	for pointer field deserialization
		/// @tparam Serializer 
		/// @tparam Accessor 
		/// @param serializer 
		/// @param field_accessor 
		/// @param  
		template <class Serializer, class Accessor>
			requires requires(Serializer serializer, Accessor field_accessor, decltype(std::declval<Accessor>().Get()) ptr, decltype(std::declval<Accessor>().Name()) name) {
				{ serializer.PointerFieldStarts(name) } -> std::same_as<void>;
				{ serializer.Deserialize(*ptr) } -> std::same_as<void>;
				{ serializer.PointerFieldEnds() } -> std::same_as<void>;
		}
		void DeserializeField(Serializer&& serializer, Accessor&& field_accessor, std::true_type) const {

			auto ptr = field_accessor.Get();
			if (!ptr) {
				InitializePointerField(field_accessor, std::bool_constant<Accessor::is_smart_pointer>());
			}

			serializer.PointerFieldStarts(field_accessor.Name());
			serializer.Deserialize(*ptr);
			serializer.PointerFieldEnds();
		}

		/// @brief	for ordinary field deserialization
		/// @tparam Serializer 
		/// @tparam Accessor 
		/// @param serializer 
		/// @param field_accessor 
		/// @param  
		template <class Serializer, class Accessor>
		void DeserializeField(Serializer&& serializer, Accessor&& field_accessor, std::false_type) const {

			typename std::decay_t<decltype(field_accessor)>::FieldType value{};
			serializer.Deserialize(field_accessor.Name(), value);
			field_accessor.Set(std::move(value));

		}


	public:
		using SelfType = Reflective<T, FieldDescriptors...>;

		explicit constexpr Reflective(T* data) noexcept : m_data(data) {}
		explicit constexpr Reflective(T& data) noexcept : m_data(&data) {}

		constexpr T* Data() noexcept { return m_data; }
		constexpr T const* Data() const noexcept { return m_data; }

		constexpr T& operator*() noexcept { return *m_data; }
		constexpr T const& operator*() const noexcept { return *m_data; }

		constexpr T* operator->() noexcept { return m_data; }
		constexpr T const* operator->() const noexcept { return m_data; }

		static consteval std::size_t FieldCount() noexcept {
			return sizeof...(FieldDescriptors);
		}

		static constexpr std::string_view TypeName() noexcept {
			return details::TypeName<T>();
		}

		template <std::size_t Index>
		constexpr auto Field() noexcept {

			static_assert(Index < sizeof...(FieldDescriptors), "Index out of range");

			using FieldType = std::tuple_element_t<Index, std::tuple<FieldDescriptors...>>;

			return FieldAccessor<T, FieldType>(m_data);

		}

		template <std::size_t Index>
		constexpr auto Field() const noexcept {

			static_assert(Index < sizeof...(FieldDescriptors), "Index out of range");

			using FieldType = std::tuple_element_t<Index, std::tuple<FieldDescriptors...>>;

			return FieldAccessor<T const, FieldType>(m_data);

		}

		template <util::FixedString Name>
		constexpr auto Field() noexcept {
			return Field<FindFieldIndex<Name>()>();
		}

		template <util::FixedString Name>
		constexpr auto Field() const noexcept {
			return Field<FindFieldIndex<Name>()>();
		}

		template <class Visitor>
		constexpr void Visit(Visitor&& visitor) noexcept {

			static constexpr std::tuple<FieldDescriptors...> fields;

			auto tuple_visitor = [this, &visitor](auto&&... field) {

				[this, &visitor] <std::size_t... Is>(
					std::index_sequence<Is...>,
					auto&&... field
					) constexpr {
					(visitor(FieldAccessor<T, std::decay_t<decltype(field)>>(m_data)), ...);
				}(std::index_sequence_for<decltype(field)...>(), std::forward<decltype(field)>(field)...);

				};

			std::apply(tuple_visitor, fields);

		}

		template <class Visitor>
		constexpr void Visit(Visitor&& visitor) const noexcept {

			static constexpr std::tuple<FieldDescriptors...> fields;

			auto tuple_visitor = [this, &visitor](auto&&... field) {

				[this, &visitor] <std::size_t... Is>(
					std::index_sequence<Is...>,
					auto&&... field
					) constexpr {
					(visitor(FieldAccessor<T const, std::decay_t<decltype(field)>>(m_data)), ...);
				}(std::index_sequence_for<decltype(field)...>(), std::forward<decltype(field)>(field)...);

				};

			std::apply(tuple_visitor, fields);

		}

		template <class Serializer>
			requires requires(Serializer serializer) {
			serializer.BeginSerialization();
			serializer.EndSerialization();
		}
		void Serialize(Serializer&& serializer) const {
			serializer.BeginSerialization();
			Visit(
				[this, &serializer](auto&& field_accessor) {

					using Accessor = std::decay_t<decltype(field_accessor)>;

					SerializeField(
						serializer, field_accessor,
						std::bool_constant<Accessor::is_pointer>{}
					);

				}
			);
			serializer.EndSerialization();
		}

		template <class Serializer>
		void Deserialize(Serializer&& serializer) {
			Visit(
				[&serializer](auto&& field_accessor) {

					using Accessor = std::decay_t<decltype(field_accessor)>;

					DeserializeField(
						serializer, field_accessor,
						std::bool_constant<Accessor::is_pointer>{}
					);

				}
			);
		}

	};

	/// @brief	This can be used to declare reflection. 
	///			For example:
	///			using Reflective = decltype(
	///				MakeReflective(
	///					std::declval<YourClass*>(), 
	///					FieldDescriptor<"name", &YourClass::name>()
	///				)
	///			);
	/// @tparam T class type
	/// @tparam ...FieldDescriptors field descriptors, must match the count of fields
	/// @param data the pointer to the instance
	/// @param ...desc field descriptors, must match the count of fields
	/// @return reflective type
	export template <class T, class... FieldDescriptors> constexpr auto MakeReflective(
		T* data,
		FieldDescriptors&&... desc
	) noexcept {
		return Reflective<T, FieldDescriptors...>{data};
	}

	/// @brief	make a reflective type of your class
	/// @tparam T class type
	/// @tparam ...FieldDescriptors field descriptors, must match the count of fields
	/// @param data the pointer to the instance
	/// @param ...desc field descriptors, must match the count of fields
	/// @return reflective type
	export template <class T, class... FieldDescriptors> constexpr auto MakeReflective(
		T& data, 
		FieldDescriptors&&... desc
	) noexcept {
		return Reflective<T, FieldDescriptors...>{data};
	}

	export template <class T> struct IsReflective 
		: std::false_type {};

	export template <class T, class... FieldDescriptors> struct IsReflective<Reflective<T, FieldDescriptors...>>
		: std::true_type {};

	export template <class T> concept ReflectiveType = IsReflective<T>::value;

}