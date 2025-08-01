﻿export module pointer_wrapper;
import std;

export namespace util {

	class NullPointer : public std::runtime_error {
	public:
		NullPointer()
			: runtime_error("attempted to dereference a null pointer") {

		}

	};

	template <class T>
	struct IsSharedPtr : std::false_type {};

	template <class T>
	struct IsSharedPtr<std::shared_ptr<T>> : std::true_type {};

	template <class T>
	struct IsWeakPtr : std::false_type {};

	template <class T>
	struct IsWeakPtr<std::weak_ptr<T>> : std::true_type {};

	/// @brief A smart pointer-like wrapper that can hold a raw pointer, std::shared_ptr, or std::weak_ptr to an object of type T, providing unified pointer semantics and access methods.
	/// @tparam T The type of the object to which the pointer refers.
	template <class T>
	class PointerWrapper {
	public:
		using Variant = std::variant<std::monostate, T*, std::shared_ptr<T>, std::weak_ptr<T>, std::unique_ptr<T>>;

	private:
		template <class U>
		friend inline PointerWrapper<U> MakeReferred(PointerWrapper<U> const& other, bool strong);

		template <class U>
		friend inline PointerWrapper<U> MakeReferred(U* other, bool strong);

		template <class U>
		friend inline PointerWrapper<U> ExtendLife(PointerWrapper<U>& other, bool move_if_unique);

		template <class U>
		friend inline PointerWrapper<U> ExtendLife(PointerWrapper<U>&& other, bool move_if_unique);

		Variant m_pointer_variant;

	public:
		Variant const& RawVariant() const noexcept {
			return m_pointer_variant;
		}

		PointerWrapper() = default;
		PointerWrapper(PointerWrapper const& other) {
			std::visit(
				[this](auto&& arg) {
					using ArgType = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<ArgType, std::unique_ptr<T>>) {
						throw std::runtime_error("Only movement of unique_ptr is allowed, not copy.");
					}
					else {
						m_pointer_variant.emplace<ArgType>(arg);
					}
				},
				other.m_pointer_variant
			);
		}

		PointerWrapper(PointerWrapper&& other) noexcept
			:m_pointer_variant(std::move(other.m_pointer_variant)) {

		}

		PointerWrapper& operator=(PointerWrapper const& other) {
			if (this != &other) {
				std::visit(
					[this](auto&& arg) {
						using ArgType = std::decay_t<decltype(arg)>;
						if constexpr (std::is_same_v<ArgType, std::unique_ptr<T>>) {
							throw std::runtime_error("Only movement of unique_ptr is allowed, not copy.");
						}
						else {
							m_pointer_variant = arg;
						}
					},
					other.m_pointer_variant
				);
			}
			return *this;
		}

		PointerWrapper& operator=(PointerWrapper&& other) noexcept {
			if (this != &other) {
				m_pointer_variant = std::move(other.m_pointer_variant);
			}
			return *this;
		}

		PointerWrapper(std::nullptr_t)
			: m_pointer_variant() {
		}

		PointerWrapper& operator=(std::nullptr_t) {
			m_pointer_variant = std::monostate();
			return *this;
		}

		PointerWrapper(T* ptr)
			:m_pointer_variant(std::in_place_type<T*>, ptr) {

		}

		PointerWrapper& operator=(T* ptr) {
			m_pointer_variant = ptr;
			return *this;
		}

		PointerWrapper(std::shared_ptr<T> const& ptr)
			:m_pointer_variant(std::in_place_type<std::shared_ptr<T>>, ptr) {

		}

		PointerWrapper& operator=(std::shared_ptr<T> const& ptr) {
			m_pointer_variant = ptr;
			return *this;
		}

		PointerWrapper(std::weak_ptr<T> const& ptr)
			:m_pointer_variant(std::in_place_type<std::weak_ptr<T>>, ptr) {

		}

		PointerWrapper& operator=(std::weak_ptr<T> const& ptr) {
			m_pointer_variant = ptr;
			return *this;
		}

		PointerWrapper(std::unique_ptr<T>&& ptr)
			:m_pointer_variant(std::in_place_type<std::unique_ptr<T>>, std::move(ptr)) {

		}

		PointerWrapper& operator=(std::unique_ptr<T>&& ptr) {
			m_pointer_variant = std::move(ptr);
			return *this;
		}

		template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		PointerWrapper(PointerWrapper<U> const& other) {
			std::visit(
				[this](auto&& arg) {
					using ArgType = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<ArgType, std::monostate>) {
						m_pointer_variant = std::monostate();
					}
					else if constexpr (std::is_same_v<ArgType, U*>) {
						m_pointer_variant = static_cast<T*>(arg);
					}
					else if constexpr (std::is_same_v<ArgType, std::shared_ptr<U>>) {
						m_pointer_variant = std::shared_ptr<T>(arg);
					}
					else if constexpr (std::is_same_v<ArgType, std::weak_ptr<U>>) {
						m_pointer_variant = std::weak_ptr<T>(arg);
					}
					else if constexpr (std::is_same_v<ArgType, std::unique_ptr<U>>) {
						throw std::runtime_error("Only movement of unique_ptr is allowed, not copy.");
					}
					else {

					}
				},
				other.RawVariant()
			);
		}

		template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		PointerWrapper& operator=(PointerWrapper<U> const& other) {
			std::visit(
				[this](auto&& arg) {
					using ArgType = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<ArgType, std::monostate>) {
						m_pointer_variant = std::monostate();
					}
					else if constexpr (std::is_same_v<ArgType, U*>) {
						m_pointer_variant = static_cast<T*>(arg);
					}
					else if constexpr (std::is_same_v<ArgType, std::shared_ptr<U>>) {
						m_pointer_variant = std::shared_ptr<T>(arg);
					}
					else if constexpr (std::is_same_v<ArgType, std::weak_ptr<U>>) {
						m_pointer_variant = std::weak_ptr<T>(arg);
					}
					else if constexpr (std::is_same_v<ArgType, std::unique_ptr<U>>) {
						throw std::runtime_error("Only movement of unique_ptr is allowed, not copy.");
					}
					else {

					}
				},
				other.RawVariant()
			);
			return *this;
		}

		template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		PointerWrapper(std::shared_ptr<U> const& ptr)
			: m_pointer_variant(std::in_place_type<std::shared_ptr<T>>, ptr) {
		}

		template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		PointerWrapper& operator=(std::shared_ptr<U> const& ptr) {
			m_pointer_variant = std::shared_ptr<T>(ptr);
			return *this;
		}

		template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		PointerWrapper(std::weak_ptr<U> const& ptr)
			: m_pointer_variant(std::in_place_type<std::weak_ptr<T>>, ptr) {
		}

		template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
		PointerWrapper& operator=(std::weak_ptr<U> const& ptr) {
			m_pointer_variant = std::weak_ptr<T>(ptr);
			return *this;
		}

		T* Get() const noexcept {
			return std::visit(
				[](auto&& arg) -> T* {
					using ArgType = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<ArgType, T*>) {
						return arg;
					}
					else if constexpr (std::is_same_v<ArgType, std::shared_ptr<T>>) {
						return arg.get();
					}
					else if constexpr (std::is_same_v<ArgType, std::weak_ptr<T>>) {
						return arg.lock().get();
					}
					else if constexpr (std::is_same_v<ArgType, std::unique_ptr<T>>) {
						return arg.get();
					}
					else {
						return nullptr;
					}
				},
				m_pointer_variant
			);
		}

		std::shared_ptr<T> Shared() const noexcept {
			return std::visit(
				[](auto&& arg) -> std::shared_ptr<T> {
					using ArgType = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<ArgType, std::shared_ptr<T>>) {
						return arg;
					}
					else if constexpr (std::is_same_v<ArgType, std::weak_ptr<T>>) {
						return arg.lock();
					}
					else {
						return nullptr;
					}
				},
				m_pointer_variant
			);
		}

	   
		T& operator*() const {
			return std::visit(
				[](auto&& arg) -> T& {
					using ArgType = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<ArgType, T*>) {
						if (!arg) {
							throw NullPointer();
						}
						return *arg;
					}
					else if constexpr (std::is_same_v<ArgType, std::shared_ptr<T>>) {
						if (!arg) {
							throw NullPointer();
						}
						return *arg;
					}
					else if constexpr (std::is_same_v<ArgType, std::weak_ptr<T>>) {
						if (auto locked = arg.lock()) {
							return *locked;
						}
						else {
							throw NullPointer();
						}
					}
					else if constexpr (std::is_same_v<ArgType, std::unique_ptr<T>>) {
						if (!arg) {
							throw NullPointer();
						}
						return *arg;
					}
					else {
						throw NullPointer();
					}
				},
				m_pointer_variant
			);
		}


		T* operator->() const noexcept {
			return Get();
		}

		explicit operator bool() const noexcept {
			return std::visit(
				[](auto&& arg) -> bool {
					using ArgType = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<ArgType, std::monostate>) {
						return false;
					}
					else if constexpr (std::is_same_v<ArgType, std::weak_ptr<T>>) {
						return !arg.expired();
					}
					else {
						return arg != nullptr;
					}

				},
				m_pointer_variant
			);
		}

		operator T*() const noexcept {
			return Get();
		}

		template <std::derived_from<T> U>
		operator U* () noexcept {
			return dynamic_cast<U*>(Get());
		}

	};

	template <class>
	struct IsPointerWrapper : std::false_type {};

	template <class T>
	struct IsPointerWrapper<PointerWrapper<T>> : std::true_type {};

	template <class T>
	inline PointerWrapper<T> MakeReferred(PointerWrapper<T> const& other, bool strong = false) {
		return std::visit(
			[strong](auto&& arg) 
			-> PointerWrapper<T> {
				using ArgType = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<ArgType, std::shared_ptr<T>>) {
					if (strong) {
						return arg;
					}
					else {
						return std::weak_ptr<T>(arg);
					}
				}
				else if constexpr (std::is_same_v<ArgType, std::unique_ptr<T>>) {
					return arg.get();
				}
				else if constexpr (std::is_same_v<ArgType, std::monostate>) {
					return nullptr;
				}
				else {
					return arg;
				}
			},
			other.m_pointer_variant
		);
	}

	template <class T>
	inline PointerWrapper<T> MakeReferred(T* other, bool strong = false) {
		return other;
	}

	template <class T>
	inline PointerWrapper<T> ExtendLife(PointerWrapper<T>& other, bool move_if_unique = true) {
		return std::visit(
			[move_if_unique](auto&& arg)
			-> PointerWrapper<T> {
				using ArgType = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<ArgType, std::weak_ptr<T>>) {
					return arg.lock();
				}
				else if constexpr (std::is_same_v<ArgType, std::unique_ptr<T>>) {
					if (move_if_unique) {
						return std::move(arg);
					}
					else {
						throw std::runtime_error("Cannot extend life of a unique_ptr without moving it.");
					}
				}
				else if constexpr (std::is_same_v<ArgType, std::monostate>) {
					return nullptr;
				}
				else {
					return arg;
				}
			},
			other.m_pointer_variant
		);
	}

	template <class T>
	inline PointerWrapper<T> ExtendLife(PointerWrapper<T>&& other, bool move_if_unique = true) {
		return std::visit(
			[move_if_unique](auto&& arg)
			-> PointerWrapper<T> {
				using ArgType = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<ArgType, std::weak_ptr<T>>) {
					return arg.lock();
				}
				else if constexpr (std::is_same_v<ArgType, std::unique_ptr<T>>) {
					if (move_if_unique) {
						return std::move(arg);
					}
					else {
						throw std::runtime_error("Cannot extend life of a unique_ptr without moving it.");
					}
				}
				else if constexpr (std::is_same_v<ArgType, std::monostate>) {
					return nullptr;
				}
				else {
					return arg;
				}
			},
			other.m_pointer_variant
		);
	}

	template <class T>
	class EnableSharedFromThis : public std::enable_shared_from_this<T> {
	public:
		using Pointer = PointerWrapper<T>;

		Pointer This() {
			try {
				return static_cast<T*>(this)->shared_from_this();
			}
			catch (std::bad_weak_ptr const&) {
				///	caller must guarantee the lifetime of this connection not managed by std::shared_ptr
				///
				return static_cast<T*>(this);
			}
		}
	};

}