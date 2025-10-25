module;
#ifdef WIN32
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <comdef.h>
#include <directx/d3dx12.h>
#endif // WIN32

export module d3d12_backend:pso;
export import rendering;
export import pointer_wrapper;
import <spirv_cross.hpp>;
import <spirv_hlsl.hpp>;
import std;
import scheduler;

namespace fyuu_engine::windows::d3d12 {
#ifdef WIN32

	export struct D3D12PipelineStateObject {
		Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;
	};

	export class D3D12PipelineStateObjectBuilder
		: public core::BasePipelineStateObjectBuilder<D3D12PipelineStateObjectBuilder, D3D12PipelineStateObject> {
		friend class core::BasePipelineStateObjectBuilder<D3D12PipelineStateObjectBuilder, D3D12PipelineStateObject>;
	private:
		std::variant<std::monostate, ID3D12Device*, Microsoft::WRL::ComPtr<ID3D12Device>> m_device;
		
		std::vector<std::byte> m_vertex_shader;
		std::vector<std::byte> m_pixel_shader;

		std::atomic_flag m_vertex_shader_ready = {};
		std::atomic_flag m_pixel_shader_ready = {};

		std::exception_ptr m_vertex_shader_exception = nullptr;
		std::exception_ptr m_pixel_shader_exception = nullptr;

		std::uint32_t m_slot = 0;
		D3D12_ROOT_SIGNATURE_FLAGS m_root_signature_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		void RethrowException() const;
		ID3D12Device* LoadDevice() const noexcept;

		static void LoadShader(
			std::filesystem::path const& path, 
			std::vector<std::byte>& output,
			std::atomic_flag& flag
		);

		D3D12PipelineStateObjectBuilder& LoadVertexShaderImpl(std::filesystem::path const& path);

		D3D12PipelineStateObjectBuilder& LoadFragmentShaderImpl(std::filesystem::path const& path);

		D3D12PipelineStateObjectBuilder& LoadPixelShaderImpl(std::filesystem::path const& path);

		static void CompileShader(
			std::filesystem::path const& path, 
			std::vector<std::byte>& output, 
			std::atomic_flag& flag,
			std::string_view entry,
			std::string_view target_profile
		);

		D3D12PipelineStateObjectBuilder& CompileVertexShaderImpl(std::filesystem::path const& path, std::string_view entry);

		D3D12PipelineStateObjectBuilder& CompileFragmentShaderImpl(std::filesystem::path const& path, std::string_view entry);

		D3D12PipelineStateObjectBuilder& CompilePixelShaderImpl(std::filesystem::path const& path, std::string_view entry);

		concurrency::AsyncTask<D3D12PipelineStateObject> BuildImpl() const;

	public:
		using Base = core::BasePipelineStateObjectBuilder<D3D12PipelineStateObjectBuilder, D3D12PipelineStateObject>;

		D3D12PipelineStateObjectBuilder();
		~D3D12PipelineStateObjectBuilder() noexcept = default;

		D3D12PipelineStateObjectBuilder& SetDevice(ID3D12Device* device);
		D3D12PipelineStateObjectBuilder& SetDevice(Microsoft::WRL::ComPtr<ID3D12Device> const& device);
		D3D12PipelineStateObjectBuilder& SetVertexInputSlot(std::uint32_t slot);
	};

	export class D3D12SPIRVPipelineStateObjectBuilder
		: public core::BasePipelineStateObjectBuilder<
		D3D12SPIRVPipelineStateObjectBuilder,
		D3D12PipelineStateObject
		>,
		public util::EnableSharedFromThis<D3D12SPIRVPipelineStateObjectBuilder> {
	private:
		friend class core::BasePipelineStateObjectBuilder<
			D3D12SPIRVPipelineStateObjectBuilder,
			D3D12PipelineStateObject
		>;

		using Base = core::BasePipelineStateObjectBuilder<
			D3D12SPIRVPipelineStateObjectBuilder,
			D3D12PipelineStateObject
		>;

		std::variant<std::monostate, ID3D12Device*, Microsoft::WRL::ComPtr<ID3D12Device>> m_device;
		std::vector<std::uint32_t> m_vertex_shader_byte_code;
		std::vector<std::uint32_t> m_fragment_shader_byte_code;
		std::uint32_t m_vertex_input_slot;

		void ProcessPushConstants(
			spirv_cross::Compiler& compiler,
			spirv_cross::ShaderResources& resources,
			std::vector<CD3DX12_ROOT_PARAMETER>& out_root_parameters
		) const;

		void ProcessResources(
			spirv_cross::Compiler& compiler,
			std::span<spirv_cross::Resource> resources,
			D3D12_DESCRIPTOR_RANGE_TYPE range_type,
			std::vector<CD3DX12_ROOT_PARAMETER>& out_root_parameters,
			std::vector<CD3DX12_DESCRIPTOR_RANGE>& out_descriptor_ranges,
			D3D12_SHADER_VISIBILITY visibility
		) const;

		void ProcessConstantBuffers(
			spirv_cross::Compiler& compiler,
			spirv_cross::ShaderResources& resources,
			std::vector<CD3DX12_ROOT_PARAMETER>& out_root_parameters,
			std::vector<CD3DX12_DESCRIPTOR_RANGE>& out_descriptor_ranges,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL
		) const;

		void ProcessTexture(
			spirv_cross::Compiler& compiler,
			spirv_cross::ShaderResources& resources,
			std::vector<CD3DX12_ROOT_PARAMETER>& out_root_parameters,
			std::vector<CD3DX12_DESCRIPTOR_RANGE>& out_descriptor_ranges,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL
		) const;

		void ProcessSampler(
			spirv_cross::Compiler& compiler,
			spirv_cross::ShaderResources& resources,
			std::vector<CD3DX12_ROOT_PARAMETER>& out_root_parameters,
			std::vector<CD3DX12_DESCRIPTOR_RANGE>& out_descriptor_ranges,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL
		) const;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(
			spirv_cross::Compiler& vertex_compiler,
			spirv_cross::ShaderResources& vertex_resource,
			spirv_cross::Compiler& fragment_compiler,
			spirv_cross::ShaderResources& fragment_resource
		) const;

		std::vector<D3D12_INPUT_ELEMENT_DESC> CreateVertexInputLayout(
			spirv_cross::Compiler& vertex_compiler,
			spirv_cross::ShaderResources& vertex_resource
		) const;

		Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
			std::span<std::uint32_t const> spirv_byte_code,
			std::string_view entry_point,
			std::string_view target_profile
		) const;

		concurrency::AsyncTask<D3D12PipelineStateObject> BuildImpl() const;

		static std::vector<std::uint32_t> ReadFile(std::filesystem::path const& path);

		D3D12SPIRVPipelineStateObjectBuilder& LoadVertexShaderImpl(std::filesystem::path const& path);

		D3D12SPIRVPipelineStateObjectBuilder& LoadFragmentShaderImpl(std::filesystem::path const& path);

	public:
		D3D12SPIRVPipelineStateObjectBuilder();

		D3D12SPIRVPipelineStateObjectBuilder& SetDevice(ID3D12Device* device);
		D3D12SPIRVPipelineStateObjectBuilder& SetDevice(Microsoft::WRL::ComPtr<ID3D12Device> const& device);
		D3D12SPIRVPipelineStateObjectBuilder& SetVertexInputSlot(std::uint32_t slot);

	};

#endif // WIN32
}