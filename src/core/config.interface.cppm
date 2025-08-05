export module config:interface;
import std;
import :node;

namespace core {

	export template <std::convertible_to<std::filesystem::path> Path>
	std::optional<std::string> ReadFileAsText(Path&& file_path) {
		std::ifstream file(std::forward<Path>(file_path));
		if (!file.is_open()) {
			return std::nullopt;
		}
		std::stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
	}

	export class IConfig {
	public:
		virtual ~IConfig() = default;
		virtual ConfigNode& Node() noexcept = 0;
		virtual bool Open(std::filesystem::path const& file_path) = 0;
		virtual bool Reopen() = 0;
		virtual bool Save() = 0;
		virtual bool SaveAs(std::filesystem::path const& file_path) = 0;

	};
}