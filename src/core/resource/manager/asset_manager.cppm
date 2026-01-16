export module core:asset_manager;
import std;
import scheduler;
import :config_manager;
import reflective;
import serialization;

namespace fyuu_engine::core::resource {

	export class AssetManager {
	private:
		concurrency::Scheduler& m_scheduler;
		std::filesystem::path m_root_path;

	public:
		AssetManager(concurrency::Scheduler& scheduler, ConfigManager& config_manager);

		template <reflection::ReflectiveConcept Reflective, reflection::SerializerConcept Serializer>
		typename Reflective::ClassType LoadAsset(std::filesystem::path const& asset, Serializer&& serializer) const {

			std::filesystem::path full_path = m_root_path / asset;
			std::ifstream file_stream(full_path, std::ios::binary);

			std::stringstream buffer;
			buffer << file_stream.rdbuf();
			serializer.Parse(buffer.str());

			typename Reflective::ClassType instance{};
			Reflective reflective(instance);

			reflective.Deserialize(serializer);

			return instance;

		}

		template <class Asset>
		concurrency::AsyncTask<Asset> AsyncLoadAsset(std::filesystem::path const& asset) const {

		}

		template <class Asset>
		concurrency::Task<Asset> AsyncLoadAsset(std::filesystem::path const& asset) const {

		}

	};


}
