export module reflective:field;
import std;
import :nameof;
export import fixed_string;
export import pointer_wrapper;

namespace fyuu_engine::reflection {

	namespace details {

		/*
		*	field pointer traits
		*/

		template <class T> 
		struct FieldPointerTraits {};

		template <class Class, class Type> 
		struct FieldPointerTraits<Type Class::*> {
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
		template <class Class, class Type> 
		struct FieldPointerTraits<Type* Class::*> {
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
		template <class Class, class Type, class Deleter> 
		struct FieldPointerTraits<std::unique_ptr<Type, Deleter> Class::*> {
			using ClassType = Class;
			using FieldType = std::unique_ptr<Type, Deleter>;
			using ActualType = Type;
			static constexpr bool is_pointer = true;
			static constexpr bool is_smart_pointer = true;
		};

		/// @brief std::shared_ptr trait
		/// @tparam Class class type
		/// @tparam Type field type
		template <class Class, class Type> 
		struct FieldPointerTraits<std::shared_ptr<Type> Class::*> {
			using ClassType = Class;
			using FieldType = std::shared_ptr<Type>;
			using ActualType = Type;
			static constexpr bool is_pointer = true;
			static constexpr bool is_smart_pointer = true;
		};

		/// @brief util::PointerWrapper trait
		/// @tparam Class class type
		/// @tparam Type field type
		template <class Class, class Type> 
		struct FieldPointerTraits<util::PointerWrapper<Type> Class::*> {
			using ClassType = Class;
			using FieldType = util::PointerWrapper<Type>;
			using ActualType = Type;
			static constexpr bool is_pointer = true;
			static constexpr bool is_smart_pointer = true;
		};

	}

	/// @brief	This is used to declare your field in the associated Reflective type
	/// @tparam Name field name
	/// @tparam MemberPtr field pointer
	export template <util::FixedString Name, auto FieldPointer> struct FieldDescriptor {
		static_assert(std::is_member_pointer_v<decltype(FieldPointer)>,
			"Invalid field pointer");

		static constexpr util::FixedString field_name = Name;
		static constexpr auto field_ptr = FieldPointer;

		using Traits = details::FieldPointerTraits<decltype(FieldPointer)>;
		using ClassType = typename Traits::ClassType;
		using FieldType = typename Traits::FieldType;

		static constexpr std::string_view type_name = NameOf<FieldType>();
	};

	export template <class T> concept FieldDescriptorConcept = requires (T t) {

		typename T::ClassType;
		typename T::FieldType;

			requires requires { T::field_name; };
			requires requires { T::type_name; };
			requires requires { T::field_ptr; };
			requires std::is_member_pointer_v<decltype(T::field_ptr)>;

	};

	/// @brief 
	/// @tparam T 
	/// @tparam FieldDesc 
	export template <class T, class FieldDesc> class FieldAccessor {
	private:
		T* m_data;

	public:
		using ClassType = typename FieldDesc::ClassType;
		using FieldType = typename FieldDesc::FieldType;
		using ActualType = typename FieldDesc::Traits::ActualType;

		static constexpr bool is_pointer = FieldDesc::Traits::is_pointer;
		static constexpr bool is_smart_pointer = FieldDesc::Traits::is_smart_pointer;

		constexpr FieldAccessor(T* data) noexcept
			: m_data(data) {
		}

		constexpr auto Get() const noexcept requires std::copy_constructible<FieldType> {
			return m_data->*(FieldDesc::field_ptr);
		}

		constexpr auto const& Get() const noexcept requires std::same_as<FieldType, std::unique_ptr<ActualType>> {
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
				return NameOf<ActualType>();
			}
			else {
				return TypeName();
			}
		}

	};

	export template <class T> concept FieldAccessorConcept = requires (T accessor) {

		typename std::decay_t<T>::FieldType;
		typename std::decay_t<T>::ActualType;
		typename std::remove_reference_t<decltype(accessor.Get())>;
		{ accessor.Set(std::declval<typename std::decay_t<T>::FieldType>()) } -> std::same_as<void>;
		{ std::decay_t<T>::Name() } -> std::convertible_to<std::string_view>;
		{ std::decay_t<T>::TypeName() } -> std::convertible_to<std::string_view>;
		{ std::decay_t<T>::ActualTypeName() } -> std::convertible_to<std::string_view>;

			requires std::same_as<decltype(std::decay_t<T>::is_pointer), bool const>;
			requires std::same_as<decltype(std::decay_t<T>::is_smart_pointer), bool const>;
			requires std::constructible_from<std::decay_t<T>, typename std::decay_t<T>::ClassType*>;

	};


}