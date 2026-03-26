/* d3d12_gpu_executable.cppm */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <utility>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <variant>
#include <optional>
#include <span>
#include <concepts>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
export module fyuu_rhi:d3d12_gpu_executable;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :enums;
import :types;
import :d3d12_common;

namespace fyuu_rhi::d3d12 {
    
    export class D3D12GPUExecutable
        : public PolymorphicGPUExecutableBase,
		public D3D12Common<D3D12GPUExecutable> {
	public:
		struct GraphicsPSO {
			Microsoft::WRL::ComPtr<ID3D12PipelineState> impl16;
			Microsoft::WRL::ComPtr<ID3D12PipelineState> impl32;
		};

		struct ComputePSO {
			Microsoft::WRL::ComPtr<ID3D12PipelineState> impl;
		};

		struct RayTracingPSO {
			Microsoft::WRL::ComPtr<ID3D12StateObject> impl;
		};

	private:
		struct Declaration {
			std::unordered_map<ShaderStage, std::optional<std::size_t>> shaders;
			std::optional<std::size_t> shader_data_segment_id;
			GPUExecutableFlags flags;
			RenderTargetAttachmentsInfo rt_att;
			std::optional<DepthBias> depth_bias;
			std::optional<DepthStencilInfo> depth_stencil;
			std::optional<MultiSample> multi_sample;
			VertexInputLayout vertex_input_layout;
			std::vector<ResourceFlags> rt_formats;
		};

		struct Compilation {
			std::optional<std::size_t> shader_data_segment_id;
			Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
			D3D12_PRIMITIVE_TOPOLOGY primitive_topology;
			std::variant<GraphicsPSO, ComputePSO, RayTracingPSO> pso;
		};

		std::optional<std::size_t> m_logical_device_id;
		std::variant<Declaration, Compilation> m_handle;

		void CompileGraphicsExecutable(Declaration* declaration);
		void CompileComputingExecutable(Declaration* declaration);
		void CompileMeshExecutable(Declaration* declaration);
		void CompileRayTracingExecutable(Declaration* declaration);

	public:
		D3D12GPUExecutable(D3D12LogicalDevice const& logical_device);

		D3D12GPUExecutable& SetShader(ShaderStage stage, D3D12Shader const& shader);
		D3D12GPUExecutable& SetShaderDataSegment(D3D12ShaderDataSegment const& shader_data_segment);
		D3D12GPUExecutable& SetFlags(GPUExecutableFlags const& flags);
		D3D12GPUExecutable& SetRenderTargetAttachments(RenderTargetAttachmentsInfo const& info);
		D3D12GPUExecutable& SetRenderTargetAttachments(std::span<BlendInfo const> blend_info, std::span<std::uint8_t const> color_write_masks);
		D3D12GPUExecutable& SetRenderTargetAttachments(LogicOp logic_op, std::span<std::uint8_t const> color_write_masks);
		D3D12GPUExecutable& SetDepthBias(DepthBias const& depth_bias);
		D3D12GPUExecutable& SetDepthStencil(DepthStencilInfo const& depth_stencil);
		D3D12GPUExecutable& SetMultiSample(MultiSample const& multi_sample);
		D3D12GPUExecutable& SetRenderTargetFormats(std::span<ResourceFlags const> format);
		D3D12GPUExecutable& SetVertexInputLayout(VertexInputLayout const& vertex_input_layout);
		D3D12GPUExecutable& SetVertexInputLayout(std::span<VertexBinding const> bindings, std::span<VertexAttribute const> attributes);
		D3D12GPUExecutable& AddVertexInputLayout(VertexBinding const& binding, VertexAttribute const& attribute);
		D3D12GPUExecutable& AddVertexBinding(VertexBinding const& binding);
		D3D12GPUExecutable& AddVertexAttribute(VertexAttribute const& attribute);

		void Compile();
		void Reset();

		std::optional<std::size_t> GetAssociatedShaderDataSegmentID() const noexcept;
		
		Microsoft::WRL::ComPtr<ID3D12RootSignature> GetAssociatedRootSignature() const noexcept;
		ID3D12RootSignature* GetRawAssociatedRootSignature() const noexcept;

		D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const noexcept;

		template <class Visitor>
		decltype(auto) VisitPSO(Visitor&& visitor) {
			auto* compilation = std::get_if<Compilation>(&m_handle);
			if (!compilation) {
				throw std::logic_error("PSO is not compiled");
			}
			static_assert(
				std::invocable<Visitor, GraphicsPSO> &&
				std::invocable<Visitor, ComputePSO> && 
				std::invocable<Visitor, RayTracingPSO>,
				"Not PSO visitor"
			);
			return std::visit(std::forward<Visitor>(visitor), compilation->pso);
		}

	};

} // namespace fyuu_rhi::d3d12

#endif // defined(_WIN32)