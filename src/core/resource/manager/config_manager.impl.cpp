module core:config_manager;

namespace fs = std::filesystem;

namespace fyuu_engine::core::resource {
	
	enum class ConfigFormat : std::uint8_t {
		Unknown,
		YAML,
		JSON
	};

	template <class Config>
	static void ValidateConfig(Config&& config) {

		std::string fallback;
		
		/*
		*	application field
		*/

		auto& application = config["application"].AsNode();
		auto& log = application["log"].AsOr(fallback);
		if (&log == &fallback) {
			throw std::runtime_error("config file: application.log not found");
		}

		/*
		*	application.window
		*/

		auto& window = application["window"].AsNode();
		auto& title = window["title"].AsOr(fallback);
		if (&title == &fallback) {
			throw std::runtime_error("config file: application.window.title not found");
		}
		std::uint32_t width = window["width"].AsOr(0);
		if (width == 0) {
			throw std::runtime_error("config file: application.window.width not found");
		}
		std::uint32_t height = window["height"].AsOr(0);
		if (height == 0) {
			throw std::runtime_error("config file: application.window.height not found");
		}

		/*
		*	application.control
		*/

		auto& control = application["control"].AsNode();
		auto& forward = control["forward"].AsOr(fallback);
		if (&forward == &fallback) {
			throw std::runtime_error("config file: application.control.forward not found");
		}
		auto& backward = control["backward"].AsOr(fallback);
		if (&backward == &fallback) {
			throw std::runtime_error("config file: application.control.backward not found");
		}
		auto& left = control["left"].AsOr(fallback);
		if (&left == &fallback) {
			throw std::runtime_error("config file: application.control.left not found");
		}
		auto& right = control["right"].AsOr(fallback);
		if (&right == &fallback) {
			throw std::runtime_error("config file: application.control.right not found");
		}
		auto& jump = control["jump"].AsOr(fallback);
		if (&jump == &fallback) {
			throw std::runtime_error("config file: application.control.jump not found");
		}
		auto& squat = control["squat"].AsOr(fallback);
		if (&squat == &fallback) {
			throw std::runtime_error("config file: application.control.squat not found");
		}
		auto& sprint = control["sprint"].AsOr(fallback);
		if (&sprint == &fallback) {
			throw std::runtime_error("config file: application.control.sprint not found");
		}
		auto& attack = control["attack"].AsOr(fallback);
		if (&attack == &fallback) {
			throw std::runtime_error("config file: application.control.attack not found");
		}
		auto& free_camera = control["free_camera"].AsOr(fallback);
		if (&free_camera == &fallback) {
			throw std::runtime_error("config file: application.control.free_camera not found");
		}

		/*
		*	engine field
		*/
		
		config::ConfigNode& engine = config["engine"].AsNode();

		auto& root = engine["root"].AsOr(fallback);
		if (&root == &fallback) {
			throw std::runtime_error("config file: engine.root not found");
		}

		auto& asset = engine["asset"].AsOr(fallback);
		if (&asset == &fallback) {
			throw std::runtime_error("config: engine.asset not found");
		}

		auto& schema = engine["schema"].AsOr(fallback);
		if (&schema == &fallback) {
			throw std::runtime_error("config: engine.schema not found");
		}

		auto& default_world = engine["default_world"].AsOr(fallback);
		if (&default_world == &fallback) {
			throw std::runtime_error("config: engine.default_world not found");
		}

		auto& big_icon = engine["big_icon"].AsOr(fallback);
		if (&big_icon == &fallback) {
			throw std::runtime_error("config: engine.big_icon not found");
		}

		auto& small_icon = engine["small_icon"].AsOr(fallback);
		if (&small_icon == &fallback) {
			throw std::runtime_error("config: engine.big_icon not found");
		}

		auto& font = engine["font"].AsOr(fallback);
		if (&font == &fallback) {
			throw std::runtime_error("config: engine.font not found");
		}

		auto& global_rendering_resource = engine["global_rendering_settings"].AsOr(fallback);
		if (&global_rendering_resource == &fallback) {
			throw std::runtime_error("config: engine.global_rendering_settings not found");
		}

		auto& global_particle_resource = engine["global_particle_setting"].AsOr(fallback);
		if (&global_particle_resource == &fallback) {
			throw std::runtime_error("config: engine.global_particle_setting not found");
		}

		auto& jolt_asset = engine["jolt_asset"].AsOr(fallback);
		if (&jolt_asset == &fallback) {
			throw std::runtime_error("config: engine.jolt_asset not found");
		}

	}

	static ConfigFormat ConfigFileFormat(fs::path const& file_path) {

		if (!fs::is_regular_file(file_path)) {
			return ConfigFormat::Unknown;
		}

		std::string extension = file_path.extension().string();
		std::transform(
			extension.begin(), extension.end(), extension.begin(),
			[](unsigned char c) { 
				return std::tolower(c); 
			}
		);

		if (extension == ".yaml" ||
			extension == ".yml") {
			return ConfigFormat::YAML;
		}

		if (extension == ".json") {
			return ConfigFormat::JSON;
		}

		return ConfigFormat::Unknown;

	}

	std::filesystem::path ConfigManager::Directory(std::string const& field1, std::string const& field2) const {
		return std::visit(
			[&field1, field2](auto&& config) -> std::filesystem::path {
				using Config = std::decay_t<decltype(config)>;
				if constexpr (std::is_same_v<std::monostate, Config>) {
					return {};
				}
				else {
					return config[field1][field2].As<std::string>();
				}
			},
			m_config
		);
	}

	ConfigManager::ConfigManager(std::filesystem::path const& config_path)
		: m_config() {

		ConfigFormat format = ConfigFileFormat(config_path);

		switch (format) {
		case ConfigFormat::YAML:
			ValidateConfig(m_config.emplace<config::YAMLConfig>());
			break;
		case ConfigFormat::JSON:
			ValidateConfig(m_config.emplace<config::JSONConfig>());
			break;
		default:
			throw std::runtime_error("Unknown configuration file format");
		}

	}

	std::filesystem::path ConfigManager::RootDirectory() const {
		return Directory("engine", "root");
	}

	std::filesystem::path ConfigManager::AssetDirectory() const {
		return Directory("engine", "asset");
	}

	std::filesystem::path ConfigManager::LogDirectory() const {
		return Directory("application", "log");
	}

	std::filesystem::path ConfigManager::GlobalRenderingSettings() const {
		return Directory("engine", "global_rendering_settings");
	}

	std::string_view ConfigManager::Title() const {
		return Field<std::string_view>("application", "window", "title");
	}

	std::uint32_t ConfigManager::Width() const {
		return Field<std::uint32_t>("application", "window", "width");
	}

	std::uint32_t ConfigManager::Height() const {
		return Field<std::uint32_t>("application", "window", "height");
	}

	application::CommandMap ConfigManager::LoadCommandMap() const noexcept {
		
		application::CommandMap map;

		auto forward = Field<std::string_view>("application", "control", "forward");
		auto backward = Field<std::string_view>("application", "control", "backward");
		auto left = Field<std::string_view>("application", "control", "left");
		auto right = Field<std::string_view>("application", "control", "right");
		auto jump = Field<std::string_view>("application", "control", "jump");
		auto squat = Field<std::string_view>("application", "control", "squat");
		auto sprint = Field<std::string_view>("application", "control", "sprint");
		auto attack = Field<std::string_view>("application", "control", "attack");
		auto free_camera = Field<std::string_view>("application", "control", "free_camera");

		map[application::KeyFromString(forward)] = application::InputCommand::Forward;
		map[application::KeyFromString(backward)] = application::InputCommand::Backward;
		map[application::KeyFromString(left)] = application::InputCommand::Left;
		map[application::KeyFromString(right)] = application::InputCommand::Right;
		map[application::KeyFromString(jump)] = application::InputCommand::Jump;
		map[application::KeyFromString(squat)] = application::InputCommand::Squat;
		map[application::KeyFromString(sprint)] = application::InputCommand::Sprint;
		map[application::KeyFromString(attack)] = application::InputCommand::Attack;
		map[application::KeyFromString(free_camera)] = application::InputCommand::FreeCamera;

		return map;
	}


}