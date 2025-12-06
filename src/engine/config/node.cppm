export module config:node;
export import arithmetic;
export import array;
import std;

namespace fyuu_engine::config {

	namespace details {
		template <class T> concept ValueConstructible = requires() {
			requires std::is_arithmetic_v<T> || std::is_convertible_v<std::string_view, T> || std::is_convertible_v<std::string, T>;
		};
	}

	export class ConfigNode {
	public:
		class Value;
		using Map = std::unordered_map<std::string, Value>;
		using Iterator = Map::iterator;
		using ConstIterator = Map::const_iterator;

	private:
		Map m_fields;

	public:
		Value& operator[](std::string const& key);
		Value const& operator[](std::string const& key) const;

		Iterator begin() noexcept;
		Iterator end() noexcept;

		ConstIterator begin() const noexcept;
		ConstIterator end() const noexcept;

		bool Contains(std::string const& field) const noexcept;

	};

	class ConfigNode::Value {
	public:
		using Array = util::Array;
		using Arithmetic = util::Arithmetic;

		enum class StorageType : std::uint8_t {
			Null,
			Arithmetic,
			String,
			Array,
			Node
		};

	private:
		using Storage = std::variant<
			std::monostate, 
			Arithmetic,
			std::string, 
			Array,
			std::unique_ptr<ConfigNode>
		>;

		Storage m_storage;

	public:
		operator bool() const noexcept;

		StorageType GetStorageType() const noexcept;

		template <class T>
		T& As() {
			return std::visit(
				[](auto&& storage) -> T& {
					using Type = std::decay_t<decltype(storage)>;
					if constexpr (std::is_same_v<Type, Arithmetic> && std::is_convertible_v<Type, T>) {
						return static_cast<T&>(storage);
					}
					else if constexpr (std::is_same_v<Type, std::string> && std::is_convertible_v<Type, T>) {
						return static_cast<T&>(storage);
					}
					else if constexpr (std::is_same_v<Type, std::unique_ptr<ConfigNode>> && std::is_convertible_v<typename std::unique_ptr<ConfigNode>::element_type, T>) {
						return static_cast<T&>(*storage);
					}
					else if constexpr (std::is_same_v<Type, Array> && std::is_convertible_v<Type, T>) {
						return static_cast<T&>(storage);
					}
					else {
						throw std::runtime_error("incompatible type or empty storage");
					}
				},
				m_storage
			);
		}


		template <class T>
		T const& As() const {
			return std::visit(
				[](auto&& storage) -> T const& {
					using Type = std::decay_t<decltype(storage)>;
					if constexpr (std::is_same_v<Type, Arithmetic> && std::is_convertible_v<Type, T>) {
						return storage;
					}
					else if constexpr (std::is_same_v<Type, std::string> && std::is_convertible_v<std::string, T>) {
						return storage;
					}
					else if constexpr (std::is_same_v<Type, std::string> && std::is_constructible_v<T, char const*>) {
						return storage.data();
					}
					else if constexpr (std::is_same_v<Type, std::unique_ptr<ConfigNode>> && std::is_convertible_v<ConfigNode, T>) {
						return *storage;
					}
					else if constexpr (std::is_same_v<Type, Array> && std::is_convertible_v<Array, T>) {
						return storage;
					}
					else {
						throw std::runtime_error("incompatible type or empty storage");
					}
				},
				m_storage
			);
		}


		template <class T>
		T& AsOr(T&& fallback) {
			return std::visit(
				[&fallback](auto&& storage) -> T& {
					using Type = std::decay_t<decltype(storage)>;
					if constexpr (std::is_same_v<Type, Arithmetic> && std::is_convertible_v<Type, T>) {
						return static_cast<T&>(storage);
					}
					else if constexpr (std::is_same_v<Type, std::string> && std::is_convertible_v<Type, T>) {
						return static_cast<T&>(storage);
					}
					else if constexpr (std::is_same_v<Type, std::string> && std::is_constructible_v<T, char const*>) {
						return storage.data();
					}
					else if constexpr (std::is_same_v<Type, std::unique_ptr<ConfigNode>> && std::is_convertible_v<Type, T>) {
						return static_cast<T&>(*storage);
					}
					else if constexpr (std::is_same_v<Type, Array> && std::is_convertible_v<Type, T>) {
						return static_cast<T&>(storage);
					}
					else {
						return fallback;
					}
				},
				m_storage
			);
		}


		template <class T>
		T const& AsOr(T&& fallback) const {
			return std::visit(
				[&fallback](auto&& storage) -> T const& {
					using Type = std::decay_t<decltype(storage)>;
					if constexpr (std::is_same_v<Type, Arithmetic> && std::is_convertible_v<Type, T>) {
						return static_cast<T const&>(storage);
					}
					else if constexpr (std::is_same_v<Type, std::string> && std::is_convertible_v<Type, T>) {
						return static_cast<T const&>(storage);
					}
					else if constexpr (std::is_same_v<Type, std::string> && std::is_constructible_v<T, char const*>) {
						return storage.data();
					}
					else if constexpr (std::is_same_v<Type, std::unique_ptr<ConfigNode>> && std::is_convertible_v<Type, T>) {
						return static_cast<T const&>(*storage);
					}
					else if constexpr (std::is_same_v<Type, Array> && std::is_convertible_v<Type, T>) {
						return static_cast<T const&>(storage);
					}
					else {
						return fallback;
					}
				},
				m_storage
			);
		}

		void Set();

		ConfigNode& AsNode();
		ConfigNode const& AsNode() const;

		Array& AsArray();
		Array const& AsArray() const;

		Arithmetic& AsArithmetic();
		Arithmetic const& AsArithmetic() const;

		void Set(Array const& array);
		void Set(Array&& array);

		template <details::ValueConstructible T>
		void Set(T&& value) {
			if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
				m_storage.emplace<Arithmetic>(std::forward<T>(value));
			}
			else if constexpr (std::is_floating_point_v<std::decay_t<T>>) {
				m_storage.emplace<Arithmetic>(std::forward<T>(value));
			}
			else if constexpr (std::is_unsigned_v<std::decay_t<T>>) {
				m_storage.emplace<Arithmetic>(std::forward<T>(value));
			}
			else if constexpr (std::is_signed_v<std::decay_t<T>>) {
				m_storage.emplace<Arithmetic>(std::forward<T>(value));
			}
			else if constexpr (std::is_convertible_v<std::decay_t<T>, std::string>) {
				m_storage.emplace<std::string>(std::forward<T>(value));
			}
			else {
				throw std::invalid_argument("invalid integral type");
			}
		}

		Value& operator[](std::string const& path);
		Value& operator[](char const* path);

		Value const& operator[](std::string const& path) const;
		Value const& operator[](char const* path) const;

	};

}