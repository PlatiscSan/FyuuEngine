#include "fyuu_graphics.h"
#include "helper_macros.h"

import std;
import fyuu_rhi;
import fyuu_engine.rhi_error;

DECLARE_IMPLEMENTATION(ShaderLibrary);

EXTERN_C DLL_API FyuuErrorCode DLL_CALL FyuuCreateShaderLibrary(
	FyuuShaderLibrary* shader_library,
	FyuuLogicalDevice* logical_device,
	char const* source,
	size_t source_size_in_bytes,
	FyuuShaderStage shader_stage,
	FyuuShaderLanguage shader_language
) NO_EXCEPT try {

	if (!shader_library || !logical_device || !source) {
		return FyuuErrorCode::FyuuErrorCode_InvalidPointer;
	}

	if (source_size_in_bytes == 0) {
		return FyuuErrorCode::FyuuErrorCode_InvalidParameter;
	}

	if (!shader_library->impl) {
		shader_library->impl = new FyuuShaderLibrary::Implementation();
	}

	auto logical_device_impl = reinterpret_cast<fyuu_rhi::LogicalDevice*>(logical_device->impl);

	return std::visit(
		[&rhi_obj = shader_library->impl->rhi_obj, source, source_size_in_bytes, shader_stage, shader_language]
		(auto&& logical_device) {
			using LogicalDevice = std::decay_t<decltype(logical_device)>;
			if constexpr (!std::same_as<std::monostate, LogicalDevice>) {

				static constexpr std::size_t index
					= fyuu_rhi::RHITypeIndex<std::remove_pointer_t<decltype(logical_device_impl)>, LogicalDevice>;

				fyuu_rhi::CompilationOptions options{};

				rhi_obj.emplace<index>(
					logical_device,
					std::string_view(source, source_size_in_bytes),
					static_cast<fyuu_rhi::ShaderStage>(shader_stage),
					static_cast<fyuu_rhi::ShaderLanguage>(shader_language),
					options
				);
				return FyuuErrorCode::FyuuErrorCode_Success;
			}
			else {
				return FyuuErrorCode::FyuuErrorCode_Unsupported;
			}
		},
		*logical_device_impl
	);

}
CATCH_COMMON_EXCEPTION(shader_library)

DECLARE_FYUU_DESTROY(ShaderLibrary)