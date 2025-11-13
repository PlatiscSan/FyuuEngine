export module reflective;
import std;
import :nameof;
export import :field;

namespace fyuu_engine::reflection {

	namespace details {
	
		template <class Serializer> concept SerializerConcept = requires(Serializer serializer) {
			{ serializer.BeginSerialization() };
			{ serializer.EndSerialization() };

		};

		template <class Serializer, class Accessor> concept Serializable = requires(Serializer serializer, Accessor accessor) {
			{ accessor.Name() } -> std::convertible_to<std::string_view>;
			{ accessor.Get() };
			{ serializer.Serialize(accessor) } -> std::same_as<void>;

		};

		template <class Serializer, class Accessor> concept Deserializable = requires(Serializer serializer, Accessor accessor) {
			{ accessor.Name() } -> std::convertible_to<std::string_view>;
			{ accessor.Get() };
			{ serializer.Deserialize(accessor) } -> std::same_as<void>;

		};

	}

	export template <class T, class... FieldDescriptors> concept ReflectiveConcept = requires () {
		requires (FieldDescriptorConcept<FieldDescriptors> && ...);
		requires (std::is_same_v<typename FieldDescriptors::ClassType, T> && ...);
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
		using ClassType = T;

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
			return NameOf<T>();
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
		constexpr void Visit(Visitor&& visitor) {

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
		constexpr void Visit(Visitor&& visitor) const {

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

		template <details::SerializerConcept Serializer>
		void Serialize(Serializer&& serializer) const {
			serializer.BeginSerialization();
			Visit(
				[this, &serializer](auto&& field_accessor) requires details::Serializable<Serializer, std::decay_t<decltype(field_accessor)>> {
					serializer.Serialize(field_accessor);
				}
			);
			serializer.EndSerialization();
		}

		template <details::SerializerConcept Serializer>
		void Deserialize(Serializer&& serializer) {
			Visit(
				[this, &serializer](auto&& field_accessor) requires details::Deserializable<Serializer, std::decay_t<decltype(field_accessor)>> {
					serializer.Deserialize(field_accessor);
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


}