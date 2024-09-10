
#include "pch.h"
#include "FileSystem.h"
#include "ThreadPool.h"
#include "Logger.h"

using namespace Fyuu::core::filesystem;
using namespace Fyuu::core::thread;

static std::list<std::filesystem::path> s_search_paths;
static std::shared_mutex s_search_paths_mutex;

void Fyuu::core::filesystem::AddSearchPath(std::filesystem::path const& path) {

	std::lock_guard<std::shared_mutex> lock(s_search_paths_mutex);

	if (std::filesystem::is_regular_file(path)) {
		throw std::runtime_error("Not a directory");
	}
	if (!std::filesystem::exists(path)) {
		std::filesystem::create_directories(path);
	}

	for (auto& search_path : s_search_paths) {
		if (search_path == path) {
			return;
		}
	}
	s_search_paths.push_back(path);

}

void Fyuu::core::filesystem::RemoveSearchPath(std::filesystem::path const& path) {

	std::lock_guard<std::shared_mutex> lock(s_search_paths_mutex);

	if (std::filesystem::is_regular_file(path)) {
		throw std::runtime_error("Not a directory");
	}

	s_search_paths.erase(
		std::remove_if(s_search_paths.begin(), s_search_paths.end(),
			[&](std::filesystem::path const& _path) { 
				return _path == path; 
			}
		)
	);

}

static FilePtr _OpenFile(std::string const& name, std::filesystem::path const& current_path) {

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

std::future<FilePtr> Fyuu::core::filesystem::OpenFile(std::string const& name) {

	return CommitAsyncTask(
		[&name]() {
			std::shared_lock<std::shared_mutex> lock(s_search_paths_mutex);
			for (auto const& search_path : s_search_paths) {
				auto file = _OpenFile(name, search_path);
				if (file) {
					return file;
				}
			}

			std::string error_msg("Failed to open file ");
			error_msg.append(name);
			log::Error(error_msg);

			return FilePtr();
		}
	);

}

std::future<std::size_t> Fyuu::core::filesystem::ReadFile(FilePtr const& file, void* buffer, std::size_t buffer_size) {

	if (!buffer || buffer_size == 0) {
		throw std::invalid_argument("Invalid buffer pointer or buffer size");
	}

	return CommitAsyncTask([&file, &buffer, &buffer_size]() {return std::fread(buffer, 1, buffer_size, file.get()); });

}

std::future<int> Fyuu::core::filesystem::FileSeek(FilePtr const& file, long offset, int pos) noexcept {

	return CommitAsyncTask([&file, &offset, &pos]() {return std::fseek(file.get(), offset, pos); });

}

std::future<std::string> Fyuu::core::filesystem::OpenAndReadText(std::string const& name) {

	return CommitAsyncTask(
		[&name]() {

			FilePtr file = nullptr;

			std::shared_lock<std::shared_mutex> lock(s_search_paths_mutex);
			for (auto const& search_path : s_search_paths) {
				file = _OpenFile(name, search_path);
				if (file) {
					break;
				}
			}

			if (!file) {

				std::string error_msg("Failed to open file ");
				error_msg.append(name);
				log::Error(error_msg);
				return std::string::basic_string();

			}

			long pos = std::ftell(file.get());
			std::fseek(file.get(), 0, SEEK_END);
			std::size_t length = std::ftell(file.get());
			std::fseek(file.get(), pos, SEEK_SET);

			std::string buffer(length + 1, 0);
			std::fread(buffer.data(), length, 1, file.get());
			buffer[length] = '\0';

			return buffer;

		}
	);

}