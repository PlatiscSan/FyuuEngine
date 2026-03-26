/* sealed_polymorphism.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <limits>
#include <concepts>
#include <type_traits>
#include <utility>
#include <stdexcept>
#include <memory>
#include <tuple>
#include <vector>
#include <variant>
#include <ranges>
#endif // !defined(__cpp_lib_modules)
export module plastic.sealed_polymorphism;

#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)

namespace plastic::utility {

	namespace details {
		template <class T>
		concept BitMaskType = std::is_unsigned_v<T> && std::is_integral_v<T>;

		struct PolymorphicBaseTag {};

	} // namespace details
	
	/**
	 * @brief A base class that enables sealed polymorphism through type bitmasks and offset calculation.
	 *
	 * This class template stores a bitmask identifying the actual derived type and the offset
	 * from the base subobject to the complete derived object. It allows safe downcasting and
	 * visitor-based dispatch without virtual functions.
	 *
	 * @tparam BitType An unsigned integral type used for the type bitmask (e.g., std::uint32_t).
	 * @tparam Ds The list of allowed derived types.
	 */
	export template <details::BitMaskType BitType, class... Ds> class PolymorphicBase 
		: public details::PolymorphicBaseTag {
	public:
		using DerivedTypesTuple = std::tuple<Ds...>;

	private:
		BitType const m_type;							///< Bitmask where exactly one bit is set, corresponding to the actual derived type.
		std::ptrdiff_t const m_offset;				///< Offset from the PolymorphicBase subobject to the complete derived object.

		static_assert(sizeof...(Ds) <= std::numeric_limits<BitType>::digits, "Not enough bits to represent all derived types");

		/**
		 * @brief Compile-time construction of a bitmask from a pack of boolean values.
		 *
		 * Each boolean value is placed at a successive bit position. For example,
		 * ToBits(true, false, true) yields a BitType value with bits 0 and 2 set.
		 *
		 * @tparam Bs Pack of boolean-like values (converted to int).
		 * @return BitType The constructed bitmask.
		 */
		template <class... Bs>
		static consteval BitType ToBits(Bs... bits) {
			BitType ret = 0;
			std::size_t pos = 0;  // Current bit index.
			((ret |= (static_cast<BitType>(bits) << pos++)), ...);  // Fold expression over the parameter pack.
			return ret;
		}

		/**
		 * @brief For a given type T, produces a bitmask where only the bit corresponding to T is set.
		 *
		 * This variable template checks T against each type in Ds and sets the bit at the matching position.
		 *
		 * @tparam T The type for which the bitmask is generated (must be one of Ds).
		 */
		template <class T>
		static constexpr BitType bitsof = ToBits(std::same_as<T, Ds>...);

		/**
		 * @brief Obtains a pointer to the complete derived object by applying the stored offset.
		 *
		 * This member function uses the explicit object parameter (deducing this) to preserve
		 * const‑qualification of the returned pointer.
		 *
		 * @tparam Self The type of the object (deduced automatically).
		 * @param self The object on which this function is called.
		 * @return A pointer to the complete derived object, with the same cv‑qualification as `self`.
		 */
		template <class Self>
		decltype(auto) DerivedPointer(this Self&& self) {
			using ReturnType = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>,
				void const*, void*>;
			return reinterpret_cast<ReturnType>(reinterpret_cast<std::intptr_t>(&self) + self.m_offset);
		}

	public:
		/**
		 * @brief Constructs a PolymorphicBase object from a pointer to a derived class.
		 *
		 * Stores the type bitmask of the derived type and computes the offset between
		 * the base subobject and the derived object.
		 *
		 * @tparam Derived The actual derived type (must be derived from PolymorphicBase).
		 * @param derived Pointer to the derived object.
		 */
		template <std::derived_from<PolymorphicBase> Derived>
		constexpr PolymorphicBase(Derived* derived) noexcept
			: m_type(bitsof<Derived>),
			m_offset(reinterpret_cast<std::intptr_t>(derived) - reinterpret_cast<std::intptr_t>(this)) {
			static_assert(!std::is_polymorphic_v<Derived>, "virtual function is banned if derived from PolymorphicBase");
		}

		constexpr PolymorphicBase(PolymorphicBase const& other) noexcept = default;
		constexpr PolymorphicBase(PolymorphicBase&& other) noexcept = default;

		/**
		 * @brief Attempts to cast the underlying object to the specified derived type.
		 *
		 * @tparam T The target type (must be one of Ds).
		 * @return Pointer to the derived object of type T if the stored type matches, otherwise nullptr.
		 */
		template <class T>
		T* As() noexcept {
			static_assert((std::same_as<T, Ds> || ...), "T must be one of the derived types");
			return (m_type & bitsof<T>) ? static_cast<T*>(this->DerivedPointer()) : nullptr;
		}

		/// @overload
		template <class T>
		T const* As() const noexcept {
			static_assert((std::same_as<T, Ds> || ...), "T must be one of the derived types");
			return (m_type & bitsof<T>) ? static_cast<T const*>(this->DerivedPointer()) : nullptr;
		}

		/**
		 * @brief Dispatches a visitor to the actual derived type.
		 *
		 * The visitor must be invocable with a pointer to each derived type in Ds,
		 * and all those invocations must return the same type.
		 *
		 * @tparam Self The type of the object (deduced).
		 * @tparam Visitor The type of the visitor callable.
		 * @param self The object on which Visit is called.
		 * @param visitor The visitor to apply.
		 * @return The result returned by the visitor.
		 * @throws std::runtime_error If the stored type does not match any of Ds (should never happen in correct usage).
		 */
		template <class Self, class Visitor>
		decltype(auto) Visit(this Self&& self, Visitor&& visitor) {
			// Determine constness of the object.
			constexpr bool is_const = std::is_const_v<std::remove_reference_t<Self>>;
		
			// Check that the visitor can be invoked with a pointer (const or non‑const) to each derived type.
			static_assert((std::invocable<Visitor, std::conditional_t<is_const, Ds const*, Ds*>> && ...),
				"Visitor must be invocable with all derived types");
		
			// Compute the return type from the first derived type (using the appropriate pointer qualifier).
			using FirstType = std::tuple_element_t<0, std::tuple<Ds...>>;
			using FirstPtr = std::conditional_t<is_const, FirstType const*, FirstType*>;
			using ResultType = decltype(std::forward<Visitor>(visitor)(std::declval<FirstPtr>()));
		
			// Ensure the visitor returns the same type for all derived types.
			static_assert((std::same_as<ResultType, decltype(std::forward<Visitor>(visitor)(std::declval<std::conditional_t<is_const, Ds const*, Ds*>>()))> && ...),
				"Visitor must return the same type for all derived types");
		
			using Base = std::conditional_t<is_const, PolymorphicBase const, PolymorphicBase>;
			Base& base = self;

			// Recursive lambda that iterates over type indices.
			auto Visit = [&base, &visitor]<std::size_t ArgumentIndex>(this auto&& self) -> ResultType {
				if constexpr (ArgumentIndex < sizeof...(Ds)) {
					using T = std::tuple_element_t<ArgumentIndex, DerivedTypesTuple>;
					if (base.m_type & bitsof<T>) {
						return std::invoke(std::forward<Visitor>(visitor), base.template As<T>());
					} else {
						return self.template operator()<ArgumentIndex + 1>();
					}
				} 
				else {
					throw std::runtime_error("PolymorphicBase::Visit: no matching derived type");
				}
				};
		
			return Visit.template operator()<0>();
		
		}

	};

	namespace details {
		template <class T> 
		concept DerivedFromPolymorphicBase = requires(T t) {
			std::derived_from<T, details::PolymorphicBaseTag>;
			typename T::DerivedTypesTuple;
		};

		template <class T>
		concept PointerLike = requires(T t) {
			*t;
			t.operator->();
		};
		
	}

	export template <class Visitor, class... Args>
	decltype(auto) Visit(Visitor&& visitor, Args&&... args) {
	
		std::tuple<Args&&...> args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);

		auto Recursive = [&visitor, &args_tuple]<std::size_t ArgumentIndex, class Self, class... Results>(this Self&& self, Results&&... results) -> decltype(auto) {
			if constexpr (ArgumentIndex == sizeof...(Args)) {
				return std::invoke(std::forward<Visitor>(visitor), std::forward<Results>(results)...);
			} 
			else {
				
				decltype(auto) arg = std::get<ArgumentIndex>(args_tuple);
				using Arg = std::decay_t<decltype(arg)>;

				if constexpr (std::is_pointer_v<Arg> || details::PointerLike<Arg>) {
					using Deref = std::decay_t<decltype(*arg)>;
					static_assert(details::DerivedFromPolymorphicBase<Deref>,
							"Visit: pointer-like argument must point to a type derived from PolymorphicBase");
					return arg->Visit(
						[&self, &results...](auto&& derived) -> decltype(auto) {
							using Derived = std::decay_t<decltype(derived)>;
							static_assert(std::is_pointer_v<Derived>,
									"Visit: derived pointer expected");
							return self.template operator()<ArgumentIndex + 1>(std::forward<Results>(results)..., derived);
						}
					);
				}
				else if constexpr (std::ranges::contiguous_range<Arg>) {
					using Element = std::decay_t<decltype(std::declval<Arg>()[0])>;
					static_assert(details::DerivedFromPolymorphicBase<std::remove_pointer_t<Element>>,
							"Visit: range element type must be derived from PolymorphicBase");
					std::size_t range_size = std::size(arg);
					if constexpr (std::is_pointer_v<Element> || details::PointerLike<Element>) {
						return arg[0]->Visit(
							[&self, range_size, &arg, &results...](auto&& derived) -> decltype(auto) {
								using Derived = std::decay_t<decltype(derived)>;
								static_assert(std::is_pointer_v<Derived>,
										"Visit: range of pointers must yield pointer");
								std::vector<Derived> deriveds;
								deriveds.reserve(range_size);
								deriveds.emplace_back(derived);
								for (std::size_t i = 1; i < range_size; ++i) {
									deriveds.emplace_back(arg[i]->template As<std::remove_const_t<std::remove_pointer_t<Derived>>>());
								}
								return self.template operator()<ArgumentIndex + 1>(std::forward<Results>(results)..., deriveds);
							}
						);
					} 
					else {
						return arg[0].Visit(
							[&self, range_size, &arg, &results...](auto&& derived) -> decltype(auto) {
								using Derived = std::decay_t<decltype(derived)>;
								static_assert(std::is_pointer_v<Derived>,
										"Visit: range of objects must produce pointer");
								std::vector<Derived> deriveds;
								deriveds.reserve(range_size);
								deriveds.emplace_back(derived);
								for (std::size_t i = 1; i < range_size; ++i) {
									deriveds.emplace_back(arg[i].template As<std::remove_const_t<std::remove_pointer_t<Derived>>>());
								}
								return self.template operator()<ArgumentIndex + 1>(std::forward<Results>(results)..., deriveds);
							}
						);
					}
				}
				else {
					static_assert(details::DerivedFromPolymorphicBase<Arg>,
								"Visit: argument must be derived from PolymorphicBase, a pointer/pointer-like to it, or a contiguous range thereof");
					return arg->Visit(
						[&self, &results...](auto&& derived) -> decltype(auto) {
							using Derived = std::decay_t<decltype(derived)>;
							static_assert(std::is_pointer_v<Derived>,
									"Visit: derived pointer expected");
							return self.template operator()<ArgumentIndex + 1>(std::forward<Results>(results)..., derived);
						}
					);
				}
			}
			};
	
		return Recursive.template operator()<0>();

	}
	/**
	 * @brief Helper for combining multiple callable objects into a single overload set.
	 *
	 * This is particularly useful for constructing visitors that handle several types.
	 *
	 * @tparam Ts The types of the callable objects to combine.
	 */
	export template <class... Ts> struct Overload : Ts... {
		using Ts::operator()...;  // Bring all operator() overloads into scope.
	};

	// Deduction guide for Overload: allows Overload{ lambda1, lambda2, ... } to deduce the template arguments.
	export template <class... Ts> Overload(Ts...) -> Overload<Ts...>;

	export struct PolymorphicGC {
		template <class Base>
		void operator()(Base* ptr) const noexcept {
			if (ptr) {
				ptr->Visit([](auto* derived) { delete derived; });
			}
		}
	};

	export template <class Base> using UniqueBase = std::unique_ptr<Base, PolymorphicGC>;

	export template <class Derived, class... Args> inline std::unique_ptr<Derived, PolymorphicGC> MakeUnique(Args&&... args) {
		return std::unique_ptr<Derived, PolymorphicGC>(new Derived(std::forward<Args>(args)...));
	}


} // namespace plastic::utility