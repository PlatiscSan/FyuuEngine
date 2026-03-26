/* d3d12_shader_data_segment.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <memory>
#include <vector>
#include <variant>
#include <optional>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
export module fyuu_rhi:d3d12_shader_data_segment;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :enums;
import :types;
import :d3d12_common;

namespace fyuu_rhi::d3d12 {

	class RootSignatureReflector {
	private:
		Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> m_deserializer;

	public:
		enum class Type : std::uint8_t {
			Unknown,
			CBV,
			UAV,
			SRV,
			Sampler,
			RootConstant 
		};

		RootSignatureReflector(std::span<std::byte const> bytes);

		std::optional<std::size_t> IndexOf(std::size_t ns, std::size_t where, Type type) const;
	};
	
    export class D3D12ShaderDataSegment
        : public PolymorphicShaderDataSegmentBase,
		public D3D12Common<D3D12ShaderDataSegment> {
	private:
		struct Declaration {
			struct RootConstantInfo {
				std::size_t count;
			};
			struct RootDescriptorInfo {
				D3D12_DESCRIPTOR_RANGE_TYPE range_type; 
			};	
			struct DescriptorTableInfo {
				std::size_t count;
				D3D12_DESCRIPTOR_RANGE_TYPE range_type;
			};
			struct StaticSamplerInfo {
				SamplerFlags flags;
				SamplerAttachmentInfo attachment;
			};
			std::size_t where;
			std::size_t ns;
			std::variant<RootConstantInfo, RootDescriptorInfo, DescriptorTableInfo, StaticSamplerInfo> info;
			D3D12_SHADER_VISIBILITY visibility;
		};

		struct InstantiatedData {
			Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
			RootSignatureReflector reflector;
		};

		std::optional<std::size_t> m_logical_device_id;
		std::variant<std::vector<Declaration>, InstantiatedData> m_handle;

	public:
		D3D12ShaderDataSegment(D3D12LogicalDevice const& logical_device);
		D3D12ShaderDataSegment& Declare(std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns = 0u);
		D3D12ShaderDataSegment& Declare(ResourceFlags flags, std::size_t where, ShaderStage visible, std::size_t ns = 0u);
		D3D12ShaderDataSegment& Declare(ResourceFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns = 0u);
		D3D12ShaderDataSegment& Declare(SamplerFlags flags, SamplerAttachmentInfo const& info, std::size_t where, ShaderStage visible, std::size_t ns = 0u);
		D3D12ShaderDataSegment& Declare(SamplerFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns = 0u);
		
		void Instantiate();
		void Reset();

		RootSignatureReflector const* GetReflector() const noexcept;

		ID3D12RootSignature* GetRawInstantiatedNative() const noexcept;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> GetInstantiatedNative() const noexcept;

	};

} // namespace fyuu_rhi::d3d12

#endif // defined(_WIN32)