export module file;
import std;

namespace fyuu_engine::util {
	export extern std::vector<std::byte> ReadFile(std::filesystem::path const& path);
}