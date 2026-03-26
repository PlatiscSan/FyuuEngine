module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <filesystem>
#include <string_view>
#include <span>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:shader;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :enums;
import :types;
import :logical_device;

namespace fs = std::filesystem;

namespace fyuu_rhi {
	export class Shader {
	private:
		UniqueShader m_impl;

	public:
		Shader(
			LogicalDevice const& logical_device,
			std::string_view src,
			ShaderStage stage,
			ShaderLanguage language,
			ShaderCompilationOption const& option
		);

		Shader(
			LogicalDevice const& logical_device,
			std::span<std::byte const> bytes
		);

		Shader(
			LogicalDevice const& logical_device,
			fs::path const& path,
			ShaderCompilationOption const& option
		);

		PolymorphicShaderBase* GetHandle() const noexcept;

	};
}