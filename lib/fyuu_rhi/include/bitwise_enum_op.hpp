#include <type_traits>
#define DEFINE_BITWISE_ENUM_OPS(EnumType)								 \
	constexpr EnumType operator|(EnumType lhs, EnumType rhs) noexcept {  \
		using T = std::underlying_type_t<EnumType>;					  \
		return static_cast<EnumType>(static_cast<T>(lhs) | static_cast<T>(rhs)); \
	}																	 \
	constexpr EnumType operator&(EnumType lhs, EnumType rhs) noexcept {  \
		using T = std::underlying_type_t<EnumType>;					  \
		return static_cast<EnumType>(static_cast<T>(lhs) & static_cast<T>(rhs)); \
	}															 \
	constexpr EnumType operator^(EnumType lhs, EnumType rhs) noexcept {  \
		using T = std::underlying_type_t<EnumType>;					  \
		return static_cast<EnumType>(static_cast<T>(lhs) ^ static_cast<T>(rhs)); \
	}																	 \
	constexpr EnumType operator~(EnumType val) noexcept {				 \
		using T = std::underlying_type_t<EnumType>;					  \
		return static_cast<EnumType>(~static_cast<T>(val));			  \
	}																	 \
	constexpr EnumType& operator|=(EnumType& lhs, EnumType rhs) noexcept { \
		return lhs = lhs | rhs;										   \
	}																	 \
	constexpr EnumType& operator&=(EnumType& lhs, EnumType rhs) noexcept { \
		return lhs = lhs & rhs;										   \
	}																	 \
	constexpr EnumType& operator^=(EnumType& lhs, EnumType rhs) noexcept { \
		return lhs = lhs ^ rhs;										   \
	}