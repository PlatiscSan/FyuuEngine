module core:asset_manager;

namespace fyuu_engine::core::resource {

	AssetManager::AssetManager(concurrency::Scheduler& scheduler, ConfigManager& config_manager) 
		: m_scheduler(scheduler),
		m_root_path(config_manager.RootDirectory()) {
	}

}
