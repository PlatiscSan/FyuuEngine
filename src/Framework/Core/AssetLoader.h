#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

#include <filesystem>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <string>

namespace Fyuu {

	class AssetLoader final {

	public:

		using FilePtr = std::unique_ptr<std::FILE, std::function<decltype(std::fclose)>>;

		void AddSearchPath(std::filesystem::path const& path);
		void RemoveSearchPath(std::filesystem::path const& path);
		FilePtr OpenFile(std::string const& name);
		std::size_t ReadFile(FilePtr const& file, void* buffer, std::size_t buffer_size) const;
		int FileSeek(FilePtr const& file, long offset, int pos) const noexcept;
		std::string OpenAndReadText(std::string const& name);

		static AssetLoader& GetInstance();
		static void DestroyInstance();

	private:

		FilePtr _OpenFile(std::string const& name, std::filesystem::path const& current_path) const;

		std::list<std::filesystem::path> m_search_paths;
		std::mutex m_mutex;

	};

}

#endif // !ASSET_LOADER_H
