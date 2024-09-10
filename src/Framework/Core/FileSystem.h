#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <filesystem>
#include <functional>
#include <future>

namespace Fyuu::core::filesystem {


	using FilePtr = std::unique_ptr<std::FILE, std::function<decltype(std::fclose)>>;

	void AddSearchPath(std::filesystem::path const& path);
	void RemoveSearchPath(std::filesystem::path const& path);
	std::future<FilePtr> OpenFile(std::string const& name);
	std::future<std::size_t> ReadFile(FilePtr const& file, void* buffer, std::size_t buffer_size);
	std::future<int> FileSeek(FilePtr const& file, long offset, int pos) noexcept;
	std::future<std::string> OpenAndReadText(std::string const& name);


}

#endif // !FILE_SYSTEM_H
