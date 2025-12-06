export module config:base;
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

}