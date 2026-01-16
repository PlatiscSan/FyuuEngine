module plastic.other;

namespace plastic::other {

	std::vector<std::byte> ReadFile(std::filesystem::path const& path) {

		std::size_t size_in_bytes = std::filesystem::file_size(path);

		std::ifstream file(path, std::ios_base::binary);
		std::vector<std::byte> byte_code(size_in_bytes);
		file.read(reinterpret_cast<char*>(byte_code.data()), size_in_bytes);

		return byte_code;

	}

}
