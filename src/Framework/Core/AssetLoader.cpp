
#include "pch.h"
#include "AssetLoader.h"
#include "Logger.h"

using namespace Fyuu;

static std::mutex s_singleton_mutex;
static std::unique_ptr<AssetLoader> s_instance = nullptr;

void Fyuu::AssetLoader::AddSearchPath(std::filesystem::path const& path) {

	std::lock_guard<std::mutex> lock(m_mutex);

	if (std::filesystem::is_regular_file(path)) {
		throw std::runtime_error("Not a directory");
	}

	for (auto& search_path : m_search_paths) {
		if (search_path == path) {
			return;
		}
	}

	m_search_paths.push_back(path);

}

void Fyuu::AssetLoader::RemoveSearchPath(std::filesystem::path const& path) {

	std::lock_guard<std::mutex> lock(m_mutex);
	if (std::filesystem::is_regular_file(path)) {
		throw std::runtime_error("Not a directory");
	}
	m_search_paths.erase(std::remove_if(m_search_paths.begin(), m_search_paths.end(), [&](std::filesystem::path const& _path) { return _path == path; }));

}

AssetLoader::FilePtr Fyuu::AssetLoader::OpenFile(std::string const& name) {

	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto const& search_path : m_search_paths) {
		auto file = _OpenFile(name, search_path);
		if (file) {
			return file;
		}
	}

	std::string error_msg("Failed to open file ");
	error_msg.append(name);
	Logger::GetInstance().Error(error_msg);

	return nullptr;
}

std::size_t Fyuu::AssetLoader::ReadFile(FilePtr const& file, void* buffer, std::size_t buffer_size) const {

	if (!buffer || buffer_size == 0) {
		throw std::invalid_argument("Invalid buffer pointer or buffer size");
	}
	
	return std::fread(buffer, buffer_size, 1, file.get());
}

int Fyuu::AssetLoader::FileSeek(FilePtr const& file, long offset, int pos) const noexcept {
	return std::fseek(file.get(), offset, pos);
}

std::string Fyuu::AssetLoader::OpenAndReadText(std::string const& name) {

	auto file = OpenFile(name);
	if (!file) {
		return std::string::basic_string();
	}
	long pos = std::ftell(file.get());
	std::fseek(file.get(), 0, SEEK_END);
	std::size_t length = std::ftell(file.get());
	std::fseek(file.get(), pos, SEEK_SET);

	std::vector<char> buffer(length + 1, 0);
	std::fread(buffer.data(), length, 1, file.get());
	buffer[length] = '\0';

	return std::string(buffer.data());

}

AssetLoader& Fyuu::AssetLoader::GetInstance() {

	std::lock_guard<std::mutex> lock(s_singleton_mutex);
	if (!s_instance) {
		try {
			s_instance.reset(new AssetLoader());
		}
		catch (std::exception const&) {
			throw std::runtime_error("Failed to initialize asset loader");
		}
	}
	return *s_instance.get();

}

void Fyuu::AssetLoader::DestroyInstance() {

	std::lock_guard<std::mutex> lock(s_singleton_mutex);
	if (s_instance) {
		s_instance.reset();
	}

}

AssetLoader::FilePtr Fyuu::AssetLoader::_OpenFile(std::string const& name, std::filesystem::path const& current_path) const {
	
	for (auto const& entry : std::filesystem::directory_iterator(current_path)) {
		if (entry.is_directory()) {
			auto file = _OpenFile(name, entry.path());
			if (file) {
				return file;
			}
		}
		else if (entry.is_regular_file() && entry.path().filename() == name) {
			FILE* fp = nullptr;
			fopen_s(&fp, entry.path().string().c_str(), "rb");
			if (fp) {
				return FilePtr(fp, std::fclose);
			}
		}
	}

	return nullptr;
}