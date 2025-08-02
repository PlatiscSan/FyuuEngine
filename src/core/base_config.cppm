export module base_config;
import std;
import defer;

export namespace core {

	template <std::convertible_to<std::filesystem::path> Path>
	std::optional<std::string> ReadFileAsText(Path&& file_path) {
		std::ifstream file(std::forward<Path>(file_path));
		if (!file.is_open()) {
			return std::nullopt;
		}
		std::stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
	}

	class Number {
	private:
		std::variant<
			std::monostate,
			std::uint8_t,
			std::uint16_t,
			std::uint32_t,
			std::uint64_t,
			std::int8_t,
			std::int16_t,
			std::int32_t,
			std::int64_t,
			bool,
			float,
			double
		> m_num;

	public:
		Number(std::monostate)
			: m_num(std::monostate()) {

		}

		template <class Value> 
			requires std::is_arithmetic_v<std::decay_t<Value>>
		Number(Value&& value)
			: m_num(std::forward<Value>(value)) {

		}

		template <class Value>
			requires std::is_arithmetic_v<std::decay_t<Value>>
		Number& operator=(Value&& value) {
			m_num.emplace<std::decay_t<Value>>(std::forward<Value>(value));
			return *this;
		}

		template <class Value>
		std::optional<Value> Get() const noexcept {
			return std::visit(
				[](auto&& num) 
				-> std::optional<Value> {
					using Storage = std::decay_t<decltype(num)>;
					if constexpr (std::is_arithmetic_v<Storage> && std::is_convertible_v<Storage, Value>) {
						return static_cast<Value>(num);
					}
					else {
						return std::nullopt;
					}
				},
				m_num
			);
		}

		auto& Variant() noexcept {
			return m_num;
		}

		auto const& Variant() const noexcept {
			return m_num;
		}

		bool IsNumber() const noexcept {
			return m_num.index() == 0;
		}

		operator bool() const noexcept {
			return !IsNumber();
		}

	};

	extern bool IsFloatingPoint(std::string_view str) {

		bool has_decimal = false;
		bool has_exponent = false;
		bool digit_before_exp = false;
		bool digit_after_exp = false;

		size_t start = 0;

		if (str[0] == '-' || str[0] == '+') {
			start = 1;
		}

		for (size_t i = start; i < str.size(); ++i) {

			char const c = str[i];

			if (c == '.') {

				if (has_decimal || has_exponent) {
					// Multiple decimal points or decimal points after index
					return false;
				}
				has_decimal = true;
			}
			else if (c == 'e' || c == 'E') {
				if (has_exponent || !digit_before_exp) {
					// Multiple indexes or no number before indexes
					return false;
				}
				has_exponent = true;
				if (i + 1 < str.size() && (str[i + 1] == '+' || str[i + 1] == '-')) {
					++i;
				}
			}
			else if (std::isdigit(static_cast<unsigned char>(c))) {
				if (has_exponent) {
					digit_after_exp = true;
				}
				else {
					digit_before_exp = true;
				}
			}
			else {
				return false;
			}
		}

		// Verify that there are numbers in the index part
		if (has_exponent && !digit_after_exp) {
			return false;
		}
		return has_decimal || has_exponent || !digit_before_exp;
	}

	template <class Value>
	std::optional<Value> TryConvertion(std::string_view str) {

		if constexpr (std::is_same_v<Value, bool>) {
			// Handle bool conversion
			if (str == "true" || str == "1") {
				return true;
			}
			else if (str == "false" || str == "0") {
				return false;
			}
			else {
				return std::nullopt;
			}
		}
		else {
			// For arithmetic types that are not bool
			Value val;
			auto result = std::from_chars(
				str.data(), str.data() + str.size(), val
			);
			if (result.ec == std::errc() &&
				result.ptr == str.data() + str.size()) {
				return val;
			}
			return std::nullopt;
		}

	}

	extern Number ConvertToNumber(std::string_view str) {

		if (str.empty()) {
			return std::monostate();
		}

		// Try integer conversion
		if (!IsFloatingPoint(str)) {
			if (auto val = TryConvertion<bool>(str)) {
				return (*val);
			}

			if (auto val = TryConvertion<std::uint8_t>(str)) {
				return *val;
			}

			if (auto val = TryConvertion<std::int8_t>(str)) {
				return *val;
			}

			if (auto val = TryConvertion<std::uint16_t>(str)) {
				return *val;
			}

			if (auto val = TryConvertion<std::int16_t>(str)) {
				return *val;
			}

			if (auto val = TryConvertion<std::uint32_t>(str)) {
				return *val;
			}

			if (auto val = TryConvertion<std::int32_t>(str)) {
				return *val;
			}

			if (auto val = TryConvertion<std::uint64_t>(str)) {
				return *val;
			}

			if (auto val = TryConvertion<std::int64_t>(str)) {
				return *val;
			}

			return std::monostate();

		}

		// Try floating point conversion

		float flt_val;
		auto flt_result = std::from_chars(
			str.data(), str.data() + str.size(), flt_val
		);

		if (flt_result.ec == std::errc() &&
			flt_result.ptr == str.data() + str.size()) {
			return flt_val;
		}

		double dbl_val;
		auto dbl_result = std::from_chars(
			str.data(), str.data() + str.size(), dbl_val
		);

		if (dbl_result.ec == std::errc() &&
			dbl_result.ptr == str.data() + str.size()) {
			return dbl_val;
		}

		return std::monostate();

	}


	class ConfigNode {
	public:
		using ConfigValue = std::variant<
			std::monostate,
			Number,
			std::vector<Number>,
			std::string,
			std::vector<std::string>,
			ConfigNode
		>;
		using Map = std::unordered_map<std::string, ConfigValue>;

	private:
		Map m_root;

		static std::vector<std::string> SplitSegment(std::string_view segment) {
			std::vector<std::string> parts;
			std::string::size_type start = 0;
			std::string::size_type end = segment.find('.');

			while (end != std::string::npos) {
				parts.emplace_back(segment.substr(start, end - start));
				start = end + 1;
				end = segment.find('.', start);
			}

			parts.emplace_back(segment.substr(start));
			return parts;
		}

		std::pair<Map*, std::string> TraverseForWrite(std::vector<std::string> const& segments) {

			if (segments.empty()) {
				throw std::invalid_argument("Empty key path");
			}

			Map* current = &m_root;
			for (size_t i = 0; i < segments.size() - 1; ++i) {
				auto const& seg = segments[i];
				auto it = current->find(seg);

				if (it == current->end()) {
					auto [new_it, _] = current->try_emplace(seg, ConfigNode());
					current = &std::get<ConfigNode>(new_it->second).m_root;
				}
				else {
					if (!std::holds_alternative<ConfigNode>(it->second)) {
						throw std::runtime_error("Path segment '" + seg + "' is not a ConfigNode");
					}
					current = &std::get<ConfigNode>(it->second).m_root;
				}
			}

			return { current, segments.back() };

		}

		std::pair<Map const*, std::string> TraverseForRead(std::vector<std::string> const& segments) const {
			if (segments.empty()) {
				return { nullptr, "" };
			}

			const Map* current = &m_root;
			for (size_t i = 0; i < segments.size() - 1; ++i) {
				auto const& seg = segments[i];
				auto it = current->find(seg);

				if (it == current->end() ||
					!std::holds_alternative<ConfigNode>(it->second)) {
					return { nullptr, "" }; 
				}
				current = &std::get<ConfigNode>(it->second).m_root;
			}
			return { current, segments.back() };
		}

	public:
		ConfigNode() = default;

		template <std::convertible_to<Map> CompatibleMap>
		ConfigNode(CompatibleMap&& map)
			: m_root(std::forward<CompatibleMap>(map)) {

		}

		Map const& Root() const noexcept {
			return m_root;
		}

		template <std::convertible_to<ConfigNode> Node>
		void Root(Node&& node) noexcept {
			m_root = std::forward<Node>(node);
		}

		template <std::convertible_to<std::string> Key>
		ConfigValue& operator[](Key&& key) {
			auto segments = SplitSegment(std::forward<Key>(key));
			if (segments.empty()) {
				throw std::invalid_argument("Empty key");
			}
			auto [last_map, last_key] = TraverseForWrite(segments);
			return last_map[last_key];
		}

		template <std::convertible_to<std::string> Key, class Value>
		void Set(Key&& key, Value&& value) {
			auto segments = SplitSegment(std::forward<Key>(key));
			if (segments.empty()) {
				throw std::invalid_argument("Empty key");
			}
			auto [last_map, last_key] = TraverseForWrite(segments);
			last_map->try_emplace(last_key, std::forward<Value>(value));
		}

		template <class Value, std::convertible_to<std::string> Key>
		std::optional<Value> Get(Key&& key) const {
			auto segments = SplitSegment(std::forward<Key>(key));
			if (segments.empty()) {
				return std::nullopt;
			}

			auto [last_map, last_key] = TraverseForRead(segments);
			if (!last_map) {
				return std::nullopt;
			}

			auto it = last_map->find(last_key);
			if (it == last_map->end()) {
				return std::nullopt;
			}

			return std::visit(
				[](auto&& value) -> std::optional<Value> {
					using Storage = std::decay_t<decltype(value)>;
					if constexpr (std::is_same_v<Storage, Number> && std::is_convertible_v<Storage, Value>) {
						return value.Get<Value>();
					}
					else if constexpr (std::is_same_v<Storage, std::vector<Number>> && std::is_convertible_v<std::vector<Number>, Value>) {
						return static_cast<Value>(value);
					}
					else if constexpr (std::is_same_v<Storage, std::string> && std::is_convertible_v<std::string, Value>) {
						return static_cast<Value>(value);
					}
					else if constexpr (std::is_same_v<Storage, std::vector<std::string>> && std::is_convertible_v<std::vector<std::string>, Value>) {
						return static_cast<Value>(value);
					}
					else {
						return std::nullopt;
					}
				},
				it->second
			);

		}

	};

	class BaseConfig {
	protected:
		ConfigNode m_node;
		std::filesystem::path m_current_config;
		std::filesystem::file_time_type m_last_modified;
		std::atomic_flag m_mutex;

		auto Lock() {
			bool mutex;
			do {
				mutex = m_mutex.test_and_set(std::memory_order::acq_rel);
				if (mutex) {
					m_mutex.wait(true, std::memory_order::relaxed);
				}
			} while (mutex);
			return util::Defer(
				[this]() {
					m_mutex.clear(std::memory_order::release);
					m_mutex.notify_one();
				}
			);
		}

	public:
		BaseConfig() = default;

		template <std::convertible_to<ConfigNode> Node>
		BaseConfig(Node&& node)
			: m_node(std::forward<Node>(node)) {

		}

		ConfigNode& Node() noexcept {
			return m_node;
		}

		virtual ~BaseConfig() = default;

		virtual bool Open(std::filesystem::path const& file_path) = 0;
		virtual bool Reopen() = 0;
		virtual bool Save() = 0;
		virtual bool SaveAs(std::filesystem::path const& file_path) = 0;

	};

}