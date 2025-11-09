export module config;
export import :node;


namespace fyuu_engine::config {

	export template <class DerivedConfig> class BaseConfig {
	protected:
		ConfigNode m_root;

	public:
		void Open(std::filesystem::path const& file_path) {
			static_cast<DerivedConfig*>(this)->OpenImpl(file_path);
		}

		void SaveAs(std::filesystem::path const& file_path) const {
			static_cast<DerivedConfig const*>(this)->SaveAsImpl(file_path);
		}

		void Parse(std::string_view dumped) {
			static_cast<DerivedConfig*>(this)->ParseImpl(dumped);
		}

		std::string Dump() const {
			return static_cast<DerivedConfig const*>(this)->DumpImpl();
		}

		ConfigNode::Value& operator[](std::string const& key) {
			return m_root[key];
		}

		ConfigNode::Value const& operator[](std::string const& key) const {
			return m_root[key];
		}

	};

	export template <class T> concept ConfigConcept = requires(T config) {
		config.Open(std::declval<std::filesystem::path>());
		config.SaveAs(std::declval<std::filesystem::path>());
		{ config[""] } -> std::convertible_to<std::reference_wrapper<ConfigNode::Value>>;
		{ config[""] } -> std::convertible_to<std::add_const_t<std::reference_wrapper<ConfigNode::Value>>>;
	};


}