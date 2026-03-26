module;
#include <version>
#if !defined(__cpp_lib_modules)
#endif // !defined(__cpp_lib_modules)
#include <nameof.hpp>
export module plastic.reflection:reflective;
#if defined(__cpp_lib_modules)
import std;
#endif

namespace plastic::reflection {

	export template <class T, class... FieldDescriptors>
	class Reflective {
	private:
		T* m_data;
	
		template <class Self>
		using CvQualifiedType = std::conditional_t<
			std::is_const_v<std::remove_reference_t<Self>>,
			T const,
			T
		>;

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
	
		template <class Self>
		constexpr auto Data(this Self&& self) noexcept 
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, T const*, T*> {
			return self.m_data;
		}
	
		template <class Self>
		constexpr auto operator*(this Self&& self) noexcept 
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, T const&, T&>  {
			return *self.m_data;
		}

		template <class Self>
		constexpr auto operator->(this Self&& self) noexcept
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, T const*, T*>  {
			return self.m_data;
		}
	
		static consteval std::size_t FieldCount() noexcept {
			return sizeof...(FieldDescriptors);
		}
	
		static constexpr std::string_view TypeName() noexcept {
			return NAMEOF(T);
		}

		template <std::size_t Index, class Self>
		constexpr auto Field(this Self&& self) noexcept {
			static_assert(Index < sizeof...(FieldDescriptors), "Index out of range");
			using FieldType = std::tuple_element_t<Index, std::tuple<FieldDescriptors...>>;
			using CvQualified = CvQualifiedType<Self>;
	
			if constexpr (std::is_const_v<std::remove_reference_t<Self>>) {
				return FieldAccessor<FieldType, CvQualified>(static_cast<T const*>(self.m_data));
			} else {
				return FieldAccessor<FieldType, CvQualified>(self.m_data);
			}
		}

		template <util::FixedString Name, class Self>
		constexpr auto Field(this Self&& self) noexcept {
			constexpr std::size_t index = FindFieldIndex<Name>();
			return self.template Field<index>();
		}

		template <class Visitor, class Self>
		constexpr void Visit(this Self&& self, Visitor&& visitor) {
			static constexpr std::tuple<FieldDescriptors...> fields;
			auto tuple_visitor = [&self, &visitor](auto&&... field) {
				[&self, &visitor] <std::size_t... Is>(
					std::index_sequence<Is...>,
					auto&&... field
				) constexpr {
					using CvQualified = CvQualifiedType<Self>;
					if constexpr (std::is_const_v<std::remove_reference_t<Self>>) {
						(visitor(FieldAccessor<std::decay_t<decltype(field)>, CvQualified>(static_cast<T const*>(self.m_data))), ...);
					} else {
						(visitor(FieldAccessor<std::decay_t<decltype(field)>, CvQualified>(self.m_data)), ...);
					}
				}(std::index_sequence_for<decltype(field)...>(), std::forward<decltype(field)>(field)...);
			};
			std::apply(tuple_visitor, fields);
		}
	};

}