module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string_view>
#endif // !defined(__cpp_lib_modules)
#include <nameof.hpp>
export module plastic.reflection:field;
#if defined(__cpp_lib_modules)
import std;
#endif
import plastic.fixed_string;

namespace plastic::reflection {

	namespace details {

		template <class T> 
		struct FieldPointerTraits {};

		template <class Class, class Type> 
		struct FieldPointerTraits<Type Class::*> {
			using ClassType = Class;
			using FieldType = Type;
			using ActualType = Type;
		};

		template <class Class, class Type> 
		struct FieldPointerTraits<Type* Class::*> {
			using ClassType = Class;
			using FieldType = Type*;
			using ActualType = Type;
		};

		template <class Class, class Type, class Deleter> 
		struct FieldPointerTraits<std::unique_ptr<Type, Deleter> Class::*> {
			using ClassType = Class;
			using FieldType = std::unique_ptr<Type, Deleter>;
			using ActualType = Type;
		};

		template <class Class, class Type> 
		struct FieldPointerTraits<std::shared_ptr<Type> Class::*> {
			using ClassType = Class;
			using FieldType = std::shared_ptr<Type>;
			using ActualType = Type;
		};


	}

	export template <util::FixedString Name, auto FieldPointer> struct FieldDescriptor {
		static_assert(std::is_member_pointer_v<decltype(FieldPointer)>,
			"Invalid field pointer");

		static constexpr util::FixedString field_name = Name;
		static constexpr auto field_ptr = FieldPointer;

		using Traits = details::FieldPointerTraits<decltype(FieldPointer)>;
		using ClassType = typename Traits::ClassType;
		using FieldType = typename Traits::FieldType;

		static constexpr std::string_view type_name = NAMEOF(FieldType);
	};


	export template <class FieldDesc, class CvQualified = typename FieldDesc::ClassType>
	class FieldAccessor {
		CvQualified* m_obj;
	
	public:
		using ClassType = typename FieldDesc::ClassType;
		using FieldType = typename FieldDesc::FieldType;
		using ActualType = typename FieldDesc::Traits::ActualType;

		constexpr FieldAccessor(CvQualified* obj) noexcept 
			: m_obj(obj) {}
	
		template <class Self>
		constexpr auto Get(this Self&& self) noexcept 
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, FieldType const&, FieldType&> {
			return self.m_obj->*FieldDesc::field_ptr;
		}
	
		template <std::convertible_to<FieldType> U>
			requires (!std::is_const_v<CvQualified>)
		FieldAccessor& operator=(U&& val) {
			Get() = std::forward<U>(val);
			return *this;
		}

		template <class U>
			requires std::convertible_to<FieldType, U> && (!std::is_const_v<CvQualified>)
		operator U() const {
			return Get();
		}
	
		static constexpr auto Name() noexcept { return FieldDesc::field_name; }
		static constexpr std::string_view TypeName() noexcept { return FieldDesc::type_name; }
	};

}