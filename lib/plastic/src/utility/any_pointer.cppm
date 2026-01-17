module;
#if defined(_WIN32)
#include <wrl.h>
#endif // defined(_WIN32)

export module plastic.any_pointer;
import std;

namespace plastic::utility {

	namespace details {
		template <class T>
		struct IsCompleteType {
			template <class U>
			static auto Test(int) -> decltype(sizeof(U), std::true_type{});

			template <class U>
			static std::false_type Test(...) {

			}

			static constexpr bool value = decltype(Test<T>(0))::value;
		};

#ifdef _WIN32
		template <class T> 
		concept IsCOMObject = requires(T t, REFIID riid, void** object) {
			{ t.AddRef() } -> std::same_as<ULONG>;
			{ t.Release() } -> std::same_as<ULONG>;
			{ t.QueryInterface(riid, object) } -> std::same_as<HRESULT>;
		};
#endif // _WIN32

		struct EmptyDeleter {
		};

	}

	export class NullPointer : public std::runtime_error {
	public:
		NullPointer()
			: runtime_error("attempted to dereference a null pointer") {

		}

	};

	export class WeakPointerException : public std::runtime_error {
	public:
		WeakPointerException()
			: runtime_error("std::weak_ptr is held and it is supposed to be converted to std::shared_ptr to use") {

		}

	};

	export class UncopiableException : public std::runtime_error {
	public:
		UncopiableException()
			: runtime_error("std::unique_ptr is held and it can be only moved") {

		}

	};

	/// @brief 
	/// @tparam T 
	export template <class T, class Deleter = std::default_delete<T>> class AnyPointer {
	private:
		using Variant = std::variant<
			T*,
			std::conditional_t<details::IsCompleteType<T>::value, std::unique_ptr<T, Deleter>, std::monostate>,
			std::shared_ptr<T>,
			std::weak_ptr<T>
		>;

		template <class U, class D>
		friend inline AnyPointer<U, D> MakeReferred(AnyPointer<U, D> const& other, bool strong);

		Variant m_ptr;
		mutable std::shared_mutex m_mutex;
		mutable std::atomic<T*> m_cache;

		template <class V, class = void>
		static Variant Copy(std::shared_mutex& mutex, V&& src) {
			std::shared_lock<std::shared_mutex> lock(mutex);
			return std::visit(
				[](auto&& ptr) -> Variant {
					using Pointer = std::decay_t<decltype(ptr)>;
					if constexpr (std::same_as<Pointer, std::unique_ptr<T, Deleter>>) {
						static_assert(details::IsCompleteType<T>::value,
							"std::unique_ptr is not available due to an incomplete type");
						throw UncopiableException();
					}
					else {
						return Variant(std::in_place_type<Pointer>, std::forward<Pointer>(ptr));
					}
				},
				src
			);
		}

		template <class V>
		static Variant Move(std::shared_mutex& mutex, V&& src) noexcept {
			std::unique_lock<std::shared_mutex> lock(mutex);
			return std::move(src);
		}


	public:
		AnyPointer(std::nullptr_t)
			: m_ptr(std::in_place_type<T*>, nullptr),
			m_cache(nullptr) {
		}

		AnyPointer() 
			: AnyPointer(nullptr) {

		}

		AnyPointer(std::monostate)
			: AnyPointer(nullptr) {

		}

		AnyPointer(AnyPointer const& other)
			: m_ptr(AnyPointer::Copy(other.m_mutex, other.m_ptr)),
			m_cache(other.m_cache.load(std::memory_order::relaxed)) {

		}

		AnyPointer(AnyPointer&& other) noexcept
			:m_ptr(AnyPointer::Move(other.m_mutex, std::move(other.m_ptr))),
			m_cache(other.m_cache.exchange(nullptr, std::memory_order::relaxed)) {
		}

		AnyPointer& operator=(AnyPointer const& other) {
			if (this != &other) {
				std::lock_guard<std::shared_mutex> lock(m_mutex);
				m_ptr = AnyPointer::Copy(other.m_mutex, other.m_ptr);
				m_cache.store(other.m_cache.load(std::memory_order::relaxed), std::memory_order::relaxed);
			}
			return *this;
		}

		AnyPointer& operator=(AnyPointer&& other) noexcept {
			if (this != &other) {
				std::lock_guard<std::shared_mutex> lock(m_mutex);
				m_ptr = AnyPointer::Move(other.m_mutex, std::move(other.m_ptr));
				m_cache.store(other.m_cache.exchange(nullptr, std::memory_order::relaxed), std::memory_order::relaxed);
			}
			return *this;
		}

		template <class U>
			requires std::convertible_to<U*, T*>
		AnyPointer(U* ptr)
			: m_ptr(std::in_place_type<T*>, ptr),
			m_cache(ptr) {
		}

		template <class U>
			requires std::convertible_to<U*, T*>
		AnyPointer& operator=(U* ptr) {
			std::lock_guard<std::shared_mutex> lock(m_mutex);
			m_ptr.emplace<T*>(ptr);
			m_cache.store(ptr, std::memory_order::relaxed);
			return *this;
		}

		template <class U>
			requires std::convertible_to<U*, T*>
		AnyPointer(std::shared_ptr<U> const& ptr)
			: m_ptr(std::in_place_type<std::shared_ptr<T>>, ptr),
			m_cache(ptr.get()) {
		}

		template <class U>
			requires std::convertible_to<U*, T*>
		AnyPointer& operator=(std::shared_ptr<U> const& ptr) {
			std::lock_guard<std::shared_mutex> lock(m_mutex);
			m_ptr.emplace<std::shared_ptr<T>>(ptr);
			m_cache.store(ptr.get(), std::memory_order::relaxed);
			return *this;
		}

		template <class U>
			requires std::convertible_to<U*, T*>
		AnyPointer(std::weak_ptr<U> const& ptr)
			: m_ptr(std::in_place_type<std::weak_ptr<T>>, ptr),
			m_cache(nullptr) {
		}

		template <class U>
			requires std::convertible_to<U*, T*>
		AnyPointer& operator=(std::weak_ptr<U> const& ptr) {
			std::lock_guard<std::shared_mutex> lock(m_mutex);
			m_ptr.emplace<std::weak_ptr<T>>(ptr);
			m_cache.store(nullptr, std::memory_order::relaxed);
			return *this;
		}

		template <class U, class D>
			requires std::convertible_to<U*, T*>
		AnyPointer(std::unique_ptr<U, D>&& ptr)
			: m_ptr(std::in_place_type<std::unique_ptr<T, Deleter>>, std::move(ptr)),
			m_cache(ptr.get()) {

			static_assert(details::IsCompleteType<T>::value,
				"std::unique_ptr is not available due to an incomplete type");

		}

		template <class U, class D>
			requires std::convertible_to<U*, T*>
		AnyPointer& operator=(std::unique_ptr<U, D>&& ptr) {

			static_assert(details::IsCompleteType<T>::value,
				"std::unique_ptr is not available due to an incomplete type");

			std::lock_guard<std::shared_mutex> lock(m_mutex);
			m_ptr.emplace<std::unique_ptr<T, Deleter>>(std::move(ptr));
			m_cache.store(nullptr, std::memory_order::relaxed);
			return *this;
		}

		AnyPointer& operator=(std::nullptr_t) {
			std::lock_guard<std::shared_mutex> lock(m_mutex);
			m_ptr.emplace<T*>(nullptr);
			m_cache.store(nullptr, std::memory_order::relaxed);
			return *this;
		}

		template <class U, class D>
			requires std::convertible_to<U*, T*>
		AnyPointer(AnyPointer<U, D> const& other)
			: m_ptr(AnyPointer::Copy(other.m_mutex, other.m_ptr)),
			m_cache(other.m_cache.load(std::memory_order::relaxed)) {

		}

		template <class U, class D>
			requires std::convertible_to<U*, T*>
		AnyPointer(AnyPointer<U, D>&& other) noexcept
			: m_ptr(AnyPointer::Move(other.m_mutex, other.m_ptr)),
			m_cache(other.m_cache.exchange(nullptr, std::memory_order::relaxed)) {

		}

		template <class U, class D>
			requires std::convertible_to<U*, T*>
		AnyPointer& operator=(AnyPointer<U, D> const& other) {
			if (this != &other) {
				std::lock_guard<std::shared_mutex> lock(m_mutex);
				m_ptr = AnyPointer::Copy(other.m_mutex, other.m_ptr);
				m_cache.store(other.m_cache.load(std::memory_order::relaxed), std::memory_order::relaxed);
			}
			return *this;
		}

		T* Get(this auto&& self) {

			if (T* cache = self.m_cache.load(std::memory_order::relaxed)) {
				return cache;
			}

			std::shared_lock<std::shared_mutex> lock(self.m_mutex);

			return std::visit(
				[&self](auto&& ptr) -> T* {
					using Pointer = std::decay_t<decltype(ptr)>;

					T* raw;

					if constexpr (std::same_as<Pointer, T*>) {
						self.m_cache.store(ptr, std::memory_order::relaxed);
						return ptr;
					}
					else if constexpr (std::same_as<Pointer, std::shared_ptr<T>>) {
						raw = ptr.get();
						self.m_cache.store(raw, std::memory_order::relaxed);
						return raw;
					}
					else if constexpr (std::same_as<Pointer, std::weak_ptr<T>>) {
						throw WeakPointerException();
					}
					else if constexpr (std::same_as<Pointer, std::unique_ptr<T, Deleter>>) {
						static_assert(details::IsCompleteType<T>::value,
							"std::unique_ptr is not available due to an incomplete type");

						raw = ptr.get();

						self.m_cache.store(raw, std::memory_order::relaxed);
						return raw;
					}
					else {
						throw NullPointer();
					}
				},
				self.m_ptr
			);

		}

		operator std::shared_ptr<T>() const {

			std::shared_lock<std::shared_mutex> lock(m_mutex);

			return std::visit(
				[](auto&& ptr) -> std::shared_ptr<T> {
					using Pointer = std::decay_t<decltype(ptr)>;
					if constexpr (std::same_as<Pointer, std::shared_ptr<T>>) {
						return ptr;
					}
					else if constexpr (std::same_as<Pointer, std::weak_ptr<T>>) {
						return ptr.lock();
					}
					else {
						throw NullPointer();
					}
				},
				m_ptr
			);
		}

		operator std::weak_ptr<T>() const {

			std::shared_lock<std::shared_mutex> lock(m_mutex);

			return std::visit(
				[](auto&& ptr) -> std::shared_ptr<T> {
					using Pointer = std::decay_t<decltype(ptr)>;
					if constexpr (std::same_as<Pointer, std::shared_ptr<T>>) {
						return ptr;
					}
					else if constexpr (std::same_as<Pointer, std::weak_ptr<T>>) {
						return ptr;
					}
					else {
						throw NullPointer();
					}
				},
				m_ptr
			);
		}

		operator std::unique_ptr<T, Deleter>() const {

			std::lock_guard<std::shared_mutex> lock(m_mutex);

			return std::visit(
				[](auto&& ptr) -> std::shared_ptr<T> {
					using Pointer = std::decay_t<decltype(ptr)>;
					if constexpr (std::same_as<Pointer, std::unique_ptr<T, Deleter>>) {
						static_assert(details::IsCompleteType<T>::value,
							"std::unique_ptr is not available due to an incomplete type");
						m_cache.store(nullptr, std::memory_order::relaxed);
						return std::move(ptr);
					}
					else {
						return {};
					}
				},
				m_ptr
			);
		}

		operator T* () {
			return Get();
		}

		template <std::derived_from<T> U>
		operator U* () {
			return static_cast<U*>(Get());
		}

		template <std::derived_from<T> U>
		operator std::shared_ptr<U>() {

			std::shared_lock<std::shared_mutex> lock(m_mutex);

			return std::visit(
				[](auto&& ptr) -> std::shared_ptr<T> {
					using Pointer = std::decay_t<decltype(ptr)>;
					if constexpr (std::same_as<Pointer, std::shared_ptr<T>>) {
						return std::static_pointer_cast<U>(ptr);
					}
					else if constexpr (std::same_as<Pointer, std::weak_ptr<T>>) {
						return std::static_pointer_cast<U>(ptr.lock());
					}
					else {
						throw NullPointer();
					}
				},
				m_ptr
			);
		}

		template <std::derived_from<T> U, class D>
		operator std::unique_ptr<U, D>() {

			std::lock_guard<std::shared_mutex> lock(m_mutex);

			return std::visit(
				[](auto&& ptr) -> std::shared_ptr<T> {
					using Pointer = std::decay_t<decltype(ptr)>;
					if constexpr (std::same_as<Pointer, std::unique_ptr<T, Deleter>>) {
						m_cache.store(nullptr, std::memory_order::relaxed);
						return std::move(ptr);
					}
					else {
						throw NullPointer();
					}
				},
				m_ptr
			);
		}

		T& operator*() const {
			return *Get();
		}

		T* operator->() const {
			return Get();
		}

		explicit operator bool() const {
			
			if (T* cache = m_cache.load(std::memory_order::relaxed)) {
				return cache;
			}

			std::shared_lock<std::shared_mutex> lock(m_mutex);

			return std::visit(
				[](auto&& ptr) -> bool {
					using Pointer = std::decay_t<decltype(ptr)>;
					if constexpr (std::same_as<Pointer, std::monostate>) {
						return false;
					}
					else if constexpr (std::same_as<Pointer, std::weak_ptr<T>>) {
						return !ptr.expired();
					}
					else {
						return ptr != nullptr;
					}

				},
				m_ptr
			);

		}

	};

#if defined(_WIN32)
	export class COMWeakPointerException : public std::runtime_error {
	public:
		COMWeakPointerException()
			: runtime_error("Microsoft::WRL::WeakRef is held and it is supposed to be converted to Microsoft::WRL::ComPtr to use") {

		}

	};

	export template <details::IsCOMObject T, class Deleter> class AnyPointer<T, Deleter> {
	private:
		using Variant = std::variant<
			std::monostate,
			Microsoft::WRL::ComPtr<T>,
			Microsoft::WRL::WeakRef
		>;

		template <details::IsCOMObject U, class D>
		friend inline AnyPointer<U, D> MakeReferred(AnyPointer<U, D> const& other, bool strong);

		Variant m_ptr;
		mutable std::shared_mutex m_mutex;
		mutable std::atomic<T*> m_cache;

		template <class V>
		static Variant Copy(std::shared_mutex& mutex, V&& src) {
			std::shared_lock<std::shared_mutex> lock(mutex);
			return src;
		}

		template <class V>
		static Variant Move(std::shared_mutex& mutex, V&& src) noexcept {
			std::unique_lock<std::shared_mutex> lock(mutex);
			return std::move(src);
		}


	public:
		AnyPointer(std::nullptr_t)
			: m_ptr(std::in_place_type<std::monostate>),
			m_cache(nullptr) {
		}

		AnyPointer()
			: AnyPointer(nullptr) {

		}

		AnyPointer(std::monostate)
			: AnyPointer(nullptr) {

		}

		AnyPointer(AnyPointer const& other)
			: m_ptr(AnyPointer::Copy(other.m_mutex, other.m_ptr)),
			m_cache(other.m_cache.load(std::memory_order::relaxed)) {

		}

		AnyPointer(AnyPointer&& other) noexcept
			: m_ptr(AnyPointer::Move(other.m_mutex, other.m_ptr)),
			m_cache(other.m_cache.exchange(nullptr, std::memory_order::relaxed)) {
		}

		AnyPointer& operator=(AnyPointer const& other) {
			if (this != &other) {
				std::lock_guard<std::shared_mutex> lock(m_mutex);
				m_ptr = AnyPointer::Copy(other.m_mutex, other.m_ptr);
				m_cache.store(other.m_cache.load(std::memory_order::relaxed), std::memory_order::relaxed);
			}
			return *this;
		}

		AnyPointer& operator=(AnyPointer&& other) noexcept {
			if (this != &other) {
				std::lock_guard<std::shared_mutex> lock(m_mutex);
				m_ptr = AnyPointer::Move(other.m_mutex, other.m_ptr);
				m_cache.store(other.m_cache.exchange(nullptr, std::memory_order::relaxed), std::memory_order::relaxed);
			}
			return *this;
		}

		template <class Pointer>
			requires std::is_constructible_v<Variant, std::decay_t<Pointer>>
		AnyPointer(Pointer&& ptr)
			: m_ptr(std::forward<Pointer>(ptr)),
			m_cache(nullptr) {

		}

		AnyPointer& operator=(std::nullptr_t) {
			std::lock_guard<std::shared_mutex> lock(m_mutex);
			m_ptr.emplace<std::monostate>();
			m_cache.store(nullptr, std::memory_order::relaxed);
			return *this;
		}

		template <class U, class D>
			requires std::convertible_to<U*, T*>
		AnyPointer(AnyPointer<U, D> const& other)
			: m_ptr(AnyPointer::Copy(other.m_mutex, other.m_ptr)),
			m_cache(other.m_cache.load(std::memory_order::relaxed)) {

		}

		template <class U, class D>
			requires std::convertible_to<U*, T*>
		AnyPointer(AnyPointer<U, D>&& other)
			: m_ptr(AnyPointer::Copy(other.m_mutex, other.m_ptr)),
			m_cache(other.m_cache.exchange(nullptr, std::memory_order::relaxed)) {

		}

		template <class Pointer>
			requires std::is_constructible_v<Variant, std::decay_t<Pointer>>
		AnyPointer& operator=(Pointer&& ptr) {
			std::lock_guard<std::shared_mutex> lock(m_mutex);
			m_ptr = std::forward<Pointer>(ptr);
			m_cache.store(nullptr, std::memory_order::relaxed);
			return *this;
		}

		template <class U, class D>
			requires std::convertible_to<U*, T*>
		AnyPointer& operator=(AnyPointer<U, D> const& other) {
			if (this != &other) {
				std::lock_guard<std::shared_mutex> lock(m_mutex);
				m_ptr = AnyPointer::Copy(other.m_mutex, other.m_ptr);
				m_cache.store(other.m_cache.load(std::memory_order::relaxed), std::memory_order::relaxed);
			}
			return *this;
		}

		T* Get(this auto&& self) {

			if (T* cache = self.m_cache.load(std::memory_order::relaxed)) {
				return cache;
			}

			std::shared_lock<std::shared_mutex> lock(self.m_mutex);

			return std::visit(
				[&self](auto&& ptr) -> T* {
					using Pointer = std::decay_t<decltype(ptr)>;
					if constexpr (std::same_as<Pointer, Microsoft::WRL::ComPtr<T>>) {
						T* raw = ptr.Get();
						self.m_cache.store(raw, std::memory_order::relaxed);
						return raw;
					}
					else if constexpr (std::same_as<Pointer, Microsoft::WRL::WeakRef> && std::derived_from<T, IInspectable>) {
						throw COMWeakPointerException();
					}
					else {
						throw NullPointer();
					}
				},
				self.m_ptr
			);

		}

		operator Microsoft::WRL::ComPtr<T>() const {

			std::shared_lock<std::shared_mutex> lock(m_mutex);

			return std::visit(
				[](auto&& ptr) -> Microsoft::WRL::ComPtr<T> {
					using Pointer = std::decay_t<decltype(ptr)>;
					if constexpr (std::same_as<Pointer, Microsoft::WRL::ComPtr<T>>) {
						return ptr;
					}
					else if constexpr (std::same_as<Pointer, Microsoft::WRL::WeakRef> && std::derived_from<T, IInspectable>) {
						Microsoft::WRL::ComPtr<T> strong;
						ptr.As(&strong);
						return strong;
					}
					else {
						throw NullPointer();
					}
				},
				m_ptr
			);
		}

		template <std::derived_from<T> U>
		operator Microsoft::WRL::ComPtr<U>() const {

			std::shared_lock<std::shared_mutex> lock(m_mutex);

			return std::visit(
				[](auto&& ptr) -> T* {
					using Pointer = std::decay_t<decltype(ptr)>;
					if constexpr (std::same_as<Pointer, Microsoft::WRL::ComPtr<T>>) {
						return ptr;
					}
					else if constexpr (std::same_as<Pointer, Microsoft::WRL::WeakRef> && std::derived_from<T, IInspectable>) {
						Microsoft::WRL::ComPtr<U> strong;
						ptr.As(&strong);
						return strong;
					}
					else {
						throw NullPointer();
					}
				},
				m_ptr
			);
		}

		operator Microsoft::WRL::WeakRef() const {

			std::shared_lock<std::shared_mutex> lock(m_mutex);

			return std::visit(
				[](auto&& ptr) -> T* {
					using Pointer = std::decay_t<decltype(ptr)>;
					if constexpr (std::same_as<Pointer, Microsoft::WRL::ComPtr<T>> && std::derived_from<T, IInspectable>) {
						Microsoft::WRL::WeakRef weak;
						ptr.AsWeak(&weak);
						return weak;
					}
					else if constexpr (std::same_as<Pointer, Microsoft::WRL::WeakRef> && std::derived_from<T, IInspectable>) {
						return ptr;
					}
					else {
						throw NullPointer();
					}
				},
				m_ptr
			);
		}

		operator T* () {
			return Get();
		}

		template <std::derived_from<T> U>
		operator U* () {
			return static_cast<U*>(Get());
		}

		T& operator*() const {
			return *Get();
		}

		T* operator->() const {
			return Get();
		}

		explicit operator bool() const {

			if (T* cache = m_cache.load(std::memory_order::relaxed)) {
				return cache;
			}

			std::shared_lock<std::shared_mutex> lock(m_mutex);

			return std::visit(
				[](auto&& ptr) -> bool {
					using Pointer = std::decay_t<decltype(ptr)>;
					if constexpr (std::same_as<Pointer, std::monostate>) {
						return false;
					}
					else if constexpr (std::same_as<Pointer, Microsoft::WRL::WeakRef> && std::derived_from<T, IInspectable>) {
						Microsoft::WRL::ComPtr<T> strong_ref;
						ptr.As(&strong_ref);
						return strong_ref;
					}
					else {
						return ptr;
					}
				},
				m_ptr
			);

		}

	};
#endif // defined(_WIN32)


	export template <class T, class Deleter = std::default_delete<T>> inline AnyPointer<T, Deleter> MakeReferred(
		AnyPointer<T, Deleter> const& other, 
		bool strong = false
	) {

		std::shared_lock<std::shared_mutex> lock(other.m_mutex);

		return std::visit(
			[strong](auto&& ptr) 
			-> AnyPointer<T, Deleter> {
				using Pointer = std::decay_t<decltype(ptr)>;
				if constexpr (std::same_as<Pointer, std::shared_ptr<T>>) {
					return strong ? ptr : std::weak_ptr<T>(ptr);
				}
				else if constexpr (std::same_as<Pointer, std::unique_ptr<T, Deleter>>) {
					static_assert(details::IsCompleteType<T>::value,
						"std::unique_ptr is not available due to an incomplete type");
					return ptr.get();
				}
				else {
					return ptr;
				}
			},
			other.m_ptr
		);
	}

	export template <details::IsCOMObject T, class Deleter> inline AnyPointer<T, Deleter> MakeReferred(
		AnyPointer<T, Deleter> const& other,
		bool strong = true
	) {

		std::shared_lock<std::shared_mutex> lock(other.m_mutex);

		return std::visit(
			[strong](auto&& ptr)
			-> AnyPointer<T, Deleter> {
				using Pointer = std::decay_t<decltype(ptr)>;
				if constexpr (std::same_as<Pointer, Microsoft::WRL::ComPtr<T>>) {

					if (strong) {
						return ptr;
					}

					if constexpr (std::derived_from<T, IInspectable>) {
						Microsoft::WRL::WeakRef weak_ref;
						ptr.AsWeak(&weak_ref);
						return weak_ref;
					}
					else {
						return ptr;
					}

				}
				else {
					return ptr;
				}
			},
			other.m_ptr
		);
	}

	export template <class Derived> class EnableSharedFromThis 
		: public std::enable_shared_from_this<Derived> {
	private:
		mutable std::once_flag m_check_flag;
		mutable std::atomic_flag m_has_valid_shared_ptr;

		bool IsManagedBySharedPtr(this auto&& self) {
			return self.weak_from_this().lock() != nullptr;
		}

	public:
		using Pointer = AnyPointer<Derived>;
		using ConstPointer = AnyPointer<Derived const>;

		auto This(this auto&& self) 
			-> std::conditional_t<std::is_const_v<std::remove_reference_t<decltype(self)>>, ConstPointer, Pointer> {

			std::call_once(
				self.m_check_flag,
				[&self]() {
					if (self.IsManagedBySharedPtr()) {
						self.m_has_valid_shared_ptr.test_and_set(std::memory_order::release);
					}
				}
			);

			if (self.m_has_valid_shared_ptr.test(std::memory_order::acquire)) {
				return self.shared_from_this();
			}
			else {
				/*
				*	caller must guarantee the lifetime of this instance not managed by std::shared_ptr
				*/
				return &self;
			}

		}

	};

}

namespace std {
	export template <class T, class Deleter> struct hash<::plastic::utility::AnyPointer<T, Deleter>> {
		std::size_t operator()(::plastic::utility::AnyPointer<T, Deleter> const& ptr) const {
			if (!ptr) {
				return 0;
			}
			return hash<T*>{}(ptr.Get());
		}
	};
}

//namespace test {
//
//	void Test() {
//		std::shared_ptr<int> shared;
//		std::weak_ptr<int> weak;
//		std::unique_ptr<int> unique;
//		plastic::utility::AnyPointer<int> any1(shared);
//		plastic::utility::AnyPointer<int> any2(weak);
//		plastic::utility::AnyPointer<int> any3(std::move(unique));
//
//	}
//
//}