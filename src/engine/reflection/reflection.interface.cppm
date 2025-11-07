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


	export template <class T> struct FieldPointerTraits {};

	export template <class Class, class Type> struct FieldPointerTraits<Type Class::*> {
		using ClassType = Class;
		using FieldType = Type;
	};

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

	export template <class T, class... FieldDescriptors>
		requires ReflectiveConcept<T, FieldDescriptors...>
	class Reflective;

	export template <class T, class FieldDesc> class FieldAccessor {
	private:
		T* m_data;

	public:
		using FieldType = typename FieldDesc::FieldType;

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
		void Serialize(Serializer&& serializer) const {
			Visit(
				[&serializer](auto&& field_accessor) {
					serializer(field_accessor.Name(), field_accessor.Get());
				}
			);
		}

		template <class Deserializer>
		void Deserialize(Deserializer&& deserializer) {
			Visit(
				[&deserializer](auto&& field_accessor) {
					typename std::decay_t<decltype(field_accessor)>::FieldType value{};
					deserializer(field_accessor.Name(), value);
					field_accessor.Set(std::move(value));
				}
			);
		}

	};

	export template <class T, class... FieldDescriptors> constexpr auto MakeReflective(
		T* data,
		FieldDescriptors&&... desc
	) noexcept {
		return Reflective<T, FieldDescriptors...>{data};
	}

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