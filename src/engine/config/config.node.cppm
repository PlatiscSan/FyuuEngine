export module config:node;
import std;

namespace fyuu_engine::config {

	export using Number = std::variant<
		std::monostate,
		std::uintmax_t,
		std::intmax_t,
		double
	>;

	export template <class T> concept Arithmetic = requires() {
		requires std::is_arithmetic_v<T>;
	};

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

	};

	class ConfigNode::Value {
	public:
		using ArrayElement = std::variant<std::monostate, Number, std::string>;
		using Array = std::vector<ArrayElement>;

		enum class StorageType : std::uint8_t {
			Null,
			Number,
			String,
			Array,
			Node
		};

	private:
		using Storage = std::variant<
			std::monostate, 
			Number,
			std::string, 
			Array,
			std::shared_ptr<ConfigNode>
		>;

		Storage m_storage;

	public:
		operator bool() const noexcept;

		StorageType GetStorageType() const noexcept;

		template <Arithmetic T>
		T Get() {
			return std::visit(
				[](auto&& storage) -> T {
					using Type = std::decay_t<decltype(storage)>;
					if constexpr (std::is_same_v<Type, Number>) {
						return std::visit(
							[](auto&& arithmetic) -> T {
								if constexpr (std::is_same_v<std::monostate, std::decay_t<decltype(arithmetic)>>) {
									throw std::runtime_error("invalid arithmetic type");
								}
								else {
									return static_cast<T>(arithmetic);
								}
							},
							storage
						);
					}
					else {
						throw std::runtime_error("incompatible type or empty storage");
					}
				},
				m_storage
			);

		}

		template <Arithmetic T>
		T Get() const {
			return std::visit(
				[](auto&& storage) -> T {
					using Type = std::decay_t<decltype(storage)>;
					if constexpr (std::is_same_v<Type, Number>) {
						return std::visit(
							[](auto&& arithmetic) -> T {
								if constexpr (std::is_same_v<std::monostate, std::decay_t<decltype(arithmetic)>>) {
									throw std::runtime_error("invalid arithmetic type");
								}
								else {
									return static_cast<T>(arithmetic);
								}
							},
							storage
						);
					}
					else {
						throw std::runtime_error("incompatible type or empty storage");
					}
				},
				m_storage
			);

		}

		template <Arithmetic T>
		T GetOr(T&& fallback) {
			return std::visit(
				[fallback](auto&& storage) -> T {
					using Type = std::decay_t<decltype(storage)>;
					if constexpr (std::is_same_v<Type, Number>) {
						return std::visit(
							[fallback](auto&& arithmetic) -> T {
								if constexpr (std::is_same_v<std::monostate, std::decay_t<decltype(arithmetic)>>) {
									return fallback;
								}
								else {
									return static_cast<T>(arithmetic);
								}
							},
							storage
						);
					}
					else {
						return fallback;
					}
				},
				m_storage
			);

		}

		template <Arithmetic T>
		T GetOr(T&& fallback) const {
			return std::visit(
				[fallback](auto&& storage) -> T {
					using Type = std::decay_t<decltype(storage)>;
					if constexpr (std::is_same_v<Type, Number>) {
						return std::visit(
							[fallback](auto&& arithmetic) -> T {
								if constexpr (std::is_same_v<std::monostate, std::decay_t<decltype(arithmetic)>>) {
									return fallback;
								}
								else {
									return static_cast<T>(arithmetic);
								}
							},
							storage
						);
					}
					else {
						return fallback;
					}
				},
				m_storage
			);

		}

		template <class T>
		T& Get() {
			return std::visit(
				[](auto&& storage) -> T& {
					using Type = std::decay_t<decltype(storage)>;
					if constexpr (std::is_same_v<Type, std::string> && std::is_convertible_v<Type, T>) {
						return static_cast<T&>(storage);
					}
					else if constexpr (std::is_same_v<Type, std::shared_ptr<ConfigNode>> && std::is_convertible_v<Type, T>) {
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
		T const& Get() const {
			return std::visit(
				[](auto&& storage) -> T const& {
					using Type = std::decay_t<decltype(storage)>;
					if constexpr (std::is_same_v<Type, std::string> && std::is_convertible_v<std::string, T>) {
						return static_cast<T const&>(storage);
					}
					else if constexpr (std::is_same_v<Type, std::shared_ptr<ConfigNode>> && std::is_convertible_v<ConfigNode, T>) {
						return static_cast<T const&>(*storage);
					}
					else if constexpr (std::is_same_v<Type, Array> && std::is_convertible_v<Array, T>) {
						return static_cast<T const&>(storage);
					}
					else {
						throw std::runtime_error("incompatible type or empty storage");
					}
				},
				m_storage
			);
		}


		template <class T>
		T& GetOr(T&& fallback) {
			return std::visit(
				[fallback](auto&& storage) -> T& {
					using Type = std::decay_t<decltype(storage)>;
					if constexpr (std::is_same_v<Type, std::string> && std::is_convertible_v<Type, T>) {
						return static_cast<T&>(storage);
					}
					else if constexpr (std::is_same_v<Type, std::string> && std::is_constructible_v<T, char const*>) {
						return storage.data();
					}
					else if constexpr (std::is_same_v<Type, std::shared_ptr<ConfigNode>> && std::is_convertible_v<Type, T>) {
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
		T const& GetOr(T&& fallback) const {
			return std::visit(
				[&fallback](auto&& storage) -> T const& {
					using Type = std::decay_t<decltype(storage)>;
					if constexpr (std::is_same_v<Type, std::string> && std::is_convertible_v<Type, T>) {
						return static_cast<T const&>(storage);
					}
					else if constexpr (std::is_same_v<Type, std::string> && std::is_constructible_v<T, char const*>) {
						return storage.data();
					}
					else if constexpr (std::is_same_v<Type, std::shared_ptr<ConfigNode>> && std::is_convertible_v<Type, T>) {
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
		void Set(std::string_view str);
		void Set(Number const& num);
		void Set(std::shared_ptr<ConfigNode> const& node);

		void AsNode();

		void Set(Array const& array);
		void Set(Array&& array);

		template <Arithmetic T>
		void Set(T&& num) {
			if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
				m_storage.emplace<Number>(std::in_place_type<std::uintmax_t>, std::forward<T>(num));
			}
			else if constexpr (std::is_floating_point_v<std::decay_t<T>>) {
				m_storage.emplace<Number>(std::in_place_type<double>, std::forward<T>(num));
			}
			else if constexpr (std::is_unsigned_v<std::decay_t<T>>) {
				m_storage.emplace<Number>(std::in_place_type<uintmax_t>, std::forward<T>(num));
			}
			else if constexpr (std::is_signed_v<std::decay_t<T>>) {
				m_storage.emplace<Number>(std::in_place_type<intmax_t>, std::forward<T>(num));
			}
			else {
				throw std::invalid_argument("invalid integral type");
			}
		}

		Value& operator[](std::string const& path);
		Value& operator[](char const* path);
		ArrayElement& operator[](std::size_t index);

		Value const& operator[](std::string const& path) const;
		Value const& operator[](char const* path) const;
		ArrayElement const& operator[](std::size_t index) const;

	};

}