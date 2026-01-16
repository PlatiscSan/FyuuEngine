module;
module fyuu_rhi;

namespace fyuu_rhi {

	std::span<API const> QueryAvailableAPI() noexcept {

		static API available_apis[] = {
#if defined(_WIN32)
			API::DirectX12,
			API::Vulkan,
			API::OpenGL,
#elif defined(__linux__)
			API::Vulkan
			API::OpenGL,
#elif defined(__APPLE__)
			API::Metal
#endif // defined(_WIN32)
		};

		return available_apis;
	}

}
