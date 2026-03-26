/* shader_datasegment.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <string>
#endif // !defined(__cpp_lib_modules)
export module fyuu_rhi:shader_data_segment;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :enums;
import :logical_device;

namespace fyuu_rhi {

	export class ShaderDataSegment {
	private:
		UniqueShaderDataSegment m_impl;

	public:
		ShaderDataSegment(LogicalDevice const& logical_device);
		ShaderDataSegment& Declare(std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns = 0u);
		ShaderDataSegment& Declare(ResourceFlags flags, std::size_t where, ShaderStage visible, std::size_t ns = 0u);
		ShaderDataSegment& Declare(ResourceFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns = 0u);
		ShaderDataSegment& Declare(SamplerFlags flags, SamplerAttachmentInfo const& info, std::size_t where, ShaderStage visible, std::size_t ns = 0u);
		ShaderDataSegment& Declare(SamplerFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns = 0u);

		void Instantiate();
		void Reset();

		PolymorphicShaderDataSegmentBase* GetHandle() const noexcept;

	};


} // namespace fyuu_rhi
