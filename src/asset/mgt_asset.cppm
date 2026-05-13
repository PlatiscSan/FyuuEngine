module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <exception>
#include <atomic>
#include <fstream>
#include <functional>
#include <filesystem>
#include <concepts>
#include <coroutine>
#include <format>
#endif // !defined(__cpp_lib_modules)
#include <boost/describe.hpp>
#include <boost/mp11.hpp>
#include <boost/uuid.hpp>
#include <tbb/concurrent_hash_map.h>
#include <tbb/task_group.h>
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>
export module fyuu_engine:managed_asset;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :asset_base;
import :asset_common;
import :log;

namespace fs = std::filesystem;

namespace {

	using namespace fyuu_engine::asset;

	tbb::task_group s_task_group;
	tbb::concurrent_hash_map<boost::uuids::uuid, AssetBase*> s_loaded_assets;

}

namespace fyuu_engine::asset {

	export enum class ConfigurationType : std::uint8_t {
		Unknown,
		YAML,
		JSON
	};

	export template <std::derived_from<AssetBase> Derived> class ManagedAsset final {
	private:
		Derived* m_impl;
		ConfigurationType m_conf_type;

		void SaveYAML() const {
			YAML::Node node;
			boost::mp11::mp_for_each<boost::describe::describe_members<Derived, boost::describe::mod_any_access>>(
				[&](auto&& desc) {
					fyuu_engine::serialization::Serialize(desc.name, m_impl->*desc.pointer, node);
				}
			);
			std::ofstream f(m_impl->conf_path);
			f << node;
		}

		void SaveJSON() const {
			nlohmann::json j;
			boost::mp11::mp_for_each<boost::describe::describe_members<Derived, boost::describe::mod_any_access>>(
				[&](auto&& desc) {
					fyuu_engine::serialization::Serialize(desc.name, m_impl->*desc.pointer, j);
				}
			);
			std::ofstream f(m_impl->conf_path);
			f << j.dump(4);
		}

	public:
		ManagedAsset(Derived* asset, ConfigurationType conf_type) noexcept
			: m_impl(asset),
			m_conf_type(conf_type) {

		}

		ManagedAsset(ManagedAsset const& other) noexcept
			: m_impl(other.m_impl),
			m_conf_type(other.m_conf_type) {
			m_impl->ref_count.fetch_add(1u, std::memory_order::relaxed);
		}

		ManagedAsset(ManagedAsset&& other) noexcept
			: m_impl(std::exchange(other.m_impl, nullptr)),
			m_conf_type(std::exchange(other.m_conf_type, ConfigurationType::Unknown)) {
		}

		~ManagedAsset() noexcept {
			try {
				if (m_impl->ref_count.fetch_sub(1u, std::memory_order::relaxed) == 1u) {
				
					if constexpr (requires{ Derived::Finalize(); }) {
						m_impl->Finalize();
					}
		
					m_impl->timestamp = std::chrono::steady_clock::now();

					switch (m_conf_type) {
					case ConfigurationType::YAML:
						SaveYAML();
						break;
					case ConfigurationType::JSON:
						SaveJSON();
						break;
					default:
						break;
					}
	
					// Remove from global cache before deletion
					s_loaded_assets.erase(m_impl->id);
					delete m_impl;
				}
			}
			catch (std::exception const& ex) {
				log::Warning(ex.what());
			}
		}

		void SaveConfiguration() const {

			m_impl->timestamp = std::chrono::steady_clock::now();

			switch (m_conf_type) {
			case ConfigurationType::YAML:
				SaveYAML();
				break;
			case ConfigurationType::JSON:
				SaveJSON();
				break;
			default:
				break;
			}

		}

		void Save() const {

			if constexpr (requires{ Derived::Save(); }) {
				m_impl->Save();
			}

			m_impl->timestamp = std::chrono::steady_clock::now();

			switch (m_conf_type) {
			case ConfigurationType::YAML:
				SaveYAML();
				break;
			case ConfigurationType::JSON:
				SaveJSON();
				break;
			default:
				break;
			}

		}

		Derived* Get() noexcept {
			return m_impl;
		} 

		Derived const* Get() const noexcept {
			return m_impl;
		} 

	};
	
	export struct AsyncFlag {};

	export template <std::derived_from<AssetBase> Derived> auto LoadRelatively(fs::path const& rel_path, AsyncFlag) {
		
		fs::path full_path = ResolveFullPath(rel_path);
		
		if (!fs::exists(full_path)) {
			std::string msg = std::format("LoadRelatively(): {} does not exist", full_path.string());
			throw std::invalid_argument(msg);
		}

		struct Awaitable {

			fs::path full_path;
			Derived* asset;
			ConfigurationType conf_type;
			std::exception_ptr ex;

			static constexpr bool await_ready() noexcept {
				return false;
			}

			void await_suspend(std::coroutine_handle<> coro) {
				s_task_group.run(
					[this, coro]() {
						try {

							asset = new Derived{};
							std::string ext = full_path.extension().string();

							if (ext == ".json") {
								std::ifstream f(full_path);
								nlohmann::json j = nlohmann::json::parse(f);
								boost::mp11::mp_for_each<boost::describe::describe_members<Derived, boost::describe::mod_any_access>>(
									[&](auto&& desc) {
										fyuu_engine::serialization::Deserialize(desc.name, asset->*desc.pointer, j);
									}
								);
								conf_type = ConfigurationType::JSON;
							}
							else if (ext == ".yaml" || ext == ".yml") {
								YAML::Node node = YAML::LoadFile(full_path.string());
								boost::mp11::mp_for_each<boost::describe::describe_members<Derived, boost::describe::mod_any_access>>(
									[&](auto&& desc) {
										fyuu_engine::serialization::Deserialize(desc.name, asset->*desc.pointer, node);
									}
								);
								conf_type = ConfigurationType::YAML;
							}
							else {
								std::string msg = std::format("LoadRelatively(): {} is an unknown type of configuration file", full_path.string());
								throw std::invalid_argument(msg);
							}

							// Store the asset path for later saving
							asset->conf_path = full_path;

							// Try to insert the asset into the cache (avoid duplicates)
							typename decltype(s_loaded_assets)::accessor acc;
							if (s_loaded_assets.insert(acc, std::make_pair(asset->id, static_cast<AssetBase*>(asset)))) {
								if constexpr (requires{ Derived::Initialize(); }) {
									asset->Initialize();
								}
							}
							else {
								// Another thread already loaded this asset; use that one instead
								auto existing_asset = static_cast<Derived*>(acc->second);
								delete asset;
								asset = existing_asset;
								conf_type = ConfigurationType::Unknown; // will be set from existing? not needed
							}

						}
						catch (std::exception const&) {
							ex = std::current_exception();
						}

						try	{
							coro();
						}
						catch (std::exception const& ex) {
							std::string msg = std::format("LoadRelatively(): Unhandled exception in coroutine, {}", ex.what());
							Error(msg);
						}

					}
				);
			}

			ManagedAsset<Derived> await_resume() {
				if (ex) {
					std::rethrow_exception(ex);
				}
				asset->ref_count.fetch_add(1u, std::memory_order::relaxed);
				return { asset, conf_type };
			}

		};

		return Awaitable{ full_path, nullptr, ConfigurationType::Unknown, nullptr };

	}

	export template <std::derived_from<AssetBase> Derived> ManagedAsset<Derived> LoadRelatively(fs::path const& rel_path) {

		fs::path full_path = ResolveFullPath(rel_path);

		if (!fs::exists(full_path)) {
			std::string msg = std::format("LoadRelatively(): {} does not exist", full_path.string());
			throw std::invalid_argument(msg);
		}

		Derived* asset = new Derived{};
		ConfigurationType conf_type = ConfigurationType::Unknown;

		try {
			std::string ext = full_path.extension().string();
			if (ext == ".json") {
				std::ifstream f(full_path);
				nlohmann::json j = nlohmann::json::parse(f);
				boost::mp11::mp_for_each<boost::describe::describe_members<Derived, boost::describe::mod_any_access>>(
					[&](auto&& desc) {
						fyuu_engine::serialization::Deserialize(desc.name, asset->*desc.pointer, j);
					}
				);
				conf_type = ConfigurationType::JSON;
			}
			else if (ext == ".yaml" || ext == ".yml") {
				YAML::Node node = YAML::LoadFile(full_path.string());
				boost::mp11::mp_for_each<boost::describe::describe_members<Derived, boost::describe::mod_any_access>>(
					[&](auto&& desc) {
						fyuu_engine::serialization::Deserialize(desc.name, asset->*desc.pointer, node);
					}
				);
				conf_type = ConfigurationType::YAML;
			}
			else {
				std::string msg = std::format("LoadRelatively(): {} is an unknown type of configuration file", full_path.string());
				throw std::invalid_argument(msg);
			}

			asset->conf_path = full_path;

			typename decltype(s_loaded_assets)::accessor acc;
			if (s_loaded_assets.insert(acc, std::make_pair(asset->id, static_cast<AssetBase*>(asset)))) {
				if constexpr (requires{ Derived::Initialize(); }) {
					asset->Initialize();
				}
			}
			else {
				// Already exists, use existing
				auto existing_asset = static_cast<Derived*>(acc->second);
				delete asset;
				asset = existing_asset;
				// conf_type may be mismatched, but existing asset already has correct type
			}
		} catch (...) {
			delete asset;
			throw;
		}

		asset->ref_count.fetch_add(1u, std::memory_order::relaxed);
		return { asset, conf_type };
	}

	export template <std::derived_from<AssetBase> Derived> ManagedAsset<Derived> CreateRelatively(
		fs::path const& rel_path,
		ConfigurationType conf_type = ConfigurationType::YAML
	) {

		fs::path full_path = ResolveFullPath(rel_path);

		// Create directories if needed
		fs::create_directories(full_path.parent_path());

		// Generate new UUID
		static thread_local boost::uuids::random_generator gen;
		boost::uuids::uuid uuid = gen();

		auto asset = new Derived{};
		asset->id = uuid;
		asset->conf_path = full_path;
		asset->timestamp = std::chrono::steady_clock::now();

		if constexpr (requires{ Derived::Initialize(); }) {
			asset->Initialize();
		}

		typename decltype(s_loaded_assets)::accessor acc;
		if (!s_loaded_assets.insert(acc, std::make_pair(uuid, static_cast<AssetBase*>(asset)))) {
			// UUID collision extremely unlikely
			delete asset;
			throw std::logic_error("CreateRelatively(): UUID collision during asset creation");
		}

		asset->ref_count.fetch_add(1u, std::memory_order::relaxed);
		return { asset, conf_type };
	}

}