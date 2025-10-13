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

module d3d12_backend:pso;
import :util;
import defer;
import win32exception;

namespace fs = std::filesystem;

namespace fyuu_engine::windows::d3d12 {
#ifdef WIN32

	struct D3D12InputElementDesc 
		: public D3D12_INPUT_ELEMENT_DESC {
		D3D12InputElementDesc& SetSemanticName(std::string_view semantic_name) {
			if (SemanticName) {
				delete[] SemanticName;
			}
			std::size_t length = semantic_name.size();
			char* ptr = new char[length + 1];
			std::strncpy(ptr, semantic_name.data(), semantic_name.size());
			ptr[length] = '\0';
			SemanticName = ptr;
			return *this;
		}
		D3D12InputElementDesc& SetSemanticIndex(std::uint32_t semantic_index) {
			SemanticIndex = semantic_index;
			return *this;
		}
		D3D12InputElementDesc& SetFormat(DXGI_FORMAT format) {
			Format = format;
			return *this;
		}

		D3D12InputElementDesc& SetInputSlot(std::uint32_t input_slot) {
			InputSlot = input_slot;
			return *this;
		}

		D3D12InputElementDesc& SetAlignedByteOffset(std::uint32_t aligned_byte_offset) {
			AlignedByteOffset = aligned_byte_offset;
			return *this;
		}

		D3D12InputElementDesc& SetInputSlotClass(D3D12_INPUT_CLASSIFICATION input_slot_class) {
			InputSlotClass = input_slot_class;
			return *this;
		}

		D3D12InputElementDesc& SetInstanceDataStepRate(std::uint32_t instance_data_step_rate) {
			InstanceDataStepRate = instance_data_step_rate;
			return *this;
		}

		D3D12InputElementDesc() {
			SemanticName = nullptr;
			SemanticIndex = 0;
			Format = DXGI_FORMAT_UNKNOWN;
			InputSlot = 0;
			AlignedByteOffset = 0;
			InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			InstanceDataStepRate = 0;
		}

		D3D12InputElementDesc(D3D12InputElementDesc const& other) {
			
			std::size_t length = std::strlen(other.SemanticName);
			char* ptr = new char[length];
			std::strncpy(ptr, other.SemanticName, length);

			SemanticName = ptr;
			SemanticIndex = other.SemanticIndex;
			Format = other.Format;
			InputSlot = other.InputSlot;
			AlignedByteOffset = other.AlignedByteOffset;
			InputSlotClass = other.InputSlotClass;
			InstanceDataStepRate = other.InstanceDataStepRate;

		}

		D3D12InputElementDesc(D3D12InputElementDesc&& other) noexcept {

			SemanticName = std::exchange(other.SemanticName, nullptr);
			SemanticIndex = std::exchange(other.SemanticIndex, 0);
			Format = std::exchange(other.Format, DXGI_FORMAT_UNKNOWN);
			InputSlot = std::exchange(other.InputSlot, 0);
			AlignedByteOffset = std::exchange(other.AlignedByteOffset, 0);
			InputSlotClass = std::exchange(other.InputSlotClass, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA);
			InstanceDataStepRate = std::exchange(other.InstanceDataStepRate, 0);

		}


		D3D12InputElementDesc& operator=(D3D12InputElementDesc const& other) {

			if (this != &other) {

				if (SemanticName) {
					delete[] SemanticName;
				}

				std::size_t length = std::strlen(other.SemanticName);
				char* ptr = new char[length];
				std::strncpy(ptr, other.SemanticName, length);

				SemanticName = ptr;
				SemanticIndex = other.SemanticIndex;
				Format = other.Format;
				InputSlot = other.InputSlot;
				AlignedByteOffset = other.AlignedByteOffset;
				InputSlotClass = other.InputSlotClass;
				InstanceDataStepRate = other.InstanceDataStepRate;

			}

			return *this;

		}

		D3D12InputElementDesc& operator=(D3D12InputElementDesc&& other) noexcept {

			if (this != &other) {

				if (SemanticName) {
					delete[] SemanticName;
				}

				SemanticName = std::exchange(other.SemanticName, nullptr);
				SemanticIndex = std::exchange(other.SemanticIndex, 0);
				Format = std::exchange(other.Format, DXGI_FORMAT_UNKNOWN);
				InputSlot = std::exchange(other.InputSlot, 0);
				AlignedByteOffset = std::exchange(other.AlignedByteOffset, 0);
				InputSlotClass = std::exchange(other.InputSlotClass, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA);
				InstanceDataStepRate = std::exchange(other.InstanceDataStepRate, 0);

			}

			return *this;

		}

		~D3D12InputElementDesc() noexcept {
			if (SemanticName) {
				delete[] SemanticName;
			}
		}
	};

	static DXGI_FORMAT SPIRTypeToDXGIFormat(spirv_cross::SPIRType const& type) noexcept {
		switch (type.basetype) {
		case spirv_cross::SPIRType::Float:
			switch (type.vecsize) {
			case 1: return DXGI_FORMAT_R32_FLOAT;
			case 2: return DXGI_FORMAT_R32G32_FLOAT;
			case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
			case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			break;

		case spirv_cross::SPIRType::Int:
			switch (type.vecsize) {
			case 1: return DXGI_FORMAT_R32_SINT;
			case 2: return DXGI_FORMAT_R32G32_SINT;
			case 3: return DXGI_FORMAT_R32G32B32_SINT;
			case 4: return DXGI_FORMAT_R32G32B32A32_SINT;
			}
			break;

		case spirv_cross::SPIRType::UInt:
			switch (type.vecsize) {
			case 1: return DXGI_FORMAT_R32_UINT;
			case 2: return DXGI_FORMAT_R32G32_UINT;
			case 3: return DXGI_FORMAT_R32G32B32_UINT;
			case 4: return DXGI_FORMAT_R32G32B32A32_UINT;
			}
			break;
		}

		return DXGI_FORMAT_UNKNOWN;
	}

	static std::uint32_t AlignedOffset(DXGI_FORMAT format) noexcept {
		switch (format) {
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return 16;

		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 12;

		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
			return 8;

		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
			return 8;

		default:
			return 4;
		}
	}

	static UINT SizeOfFormat(DXGI_FORMAT format) noexcept {
		switch (format) {
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return 16;

		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 12;

		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return 16;

		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
			return 8;

		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
			return 8;

		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
			return 4;

		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
			return 4;

		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
			return 4;

		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
			return 2;

		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_D16_UNORM:
			return 2;

		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
			return 1;

		default:
			return 0;
		}
	}

	static std::vector<std::byte> ReadFile(std::filesystem::path const& path) {

		std::size_t size_in_bytes = fs::file_size(path);

		std::ifstream file(path, std::ios_base::binary);
		std::vector<std::byte> byte_code(size_in_bytes);
		file.read(reinterpret_cast<char*>(byte_code.data()), size_in_bytes);

		return byte_code;

	}

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		std::span<std::byte> source,
		std::string_view entry,
		std::string_view target_profile
	) {

		UINT compile_flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
		compile_flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG
		Microsoft::WRL::ComPtr<ID3DBlob> shader_blob;
		Microsoft::WRL::ComPtr<ID3DBlob> error_blob;
		HRESULT result = D3DCompile(
			source.data(),
			source.size(),
			nullptr,
			nullptr,
			nullptr,
			entry.data(),
			target_profile.data(),
			compile_flags,
			0,
			&shader_blob,
			&error_blob
		);

		if (FAILED(result)) {
			if (error_blob) {
				std::string error_message(static_cast<char const*>(error_blob->GetBufferPointer()), error_blob->GetBufferSize());
				throw std::runtime_error("D3DCompile failed: " + error_message);
			}
			else {
				throw _com_error(result);
			}
		}

		return shader_blob;

	}

	static std::vector<D3D12InputElementDesc> ReflectInputLayout(
		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> const& shader_reflection, 
		std::uint32_t slot
	) {

		static DXGI_FORMAT const FormatTable[4][3] = {
			// UINT32,							SINT32,							FLOAT32
			{ DXGI_FORMAT_R32_UINT,				DXGI_FORMAT_R32_SINT,			DXGI_FORMAT_R32_FLOAT  },			// Mask == 1
			{ DXGI_FORMAT_R32G32_UINT,			DXGI_FORMAT_R32G32_SINT,		DXGI_FORMAT_R32G32_FLOAT },			// Mask <= 3
			{ DXGI_FORMAT_R32G32B32_UINT,		DXGI_FORMAT_R32G32B32_SINT,		DXGI_FORMAT_R32G32B32_FLOAT },		// Mask <= 7
			{ DXGI_FORMAT_R32G32B32A32_UINT,	DXGI_FORMAT_R32G32B32A32_SINT,	DXGI_FORMAT_R32G32B32A32_FLOAT }	// Mask <= 15
		};

		D3D12_SHADER_DESC shader_desc;
		ThrowIfFailed(shader_reflection->GetDesc(&shader_desc));

		std::vector<D3D12InputElementDesc> input_layout;
		input_layout.reserve(shader_desc.InputParameters);
		std::uint32_t offset = 0;

		for (std::uint32_t i = 0; i < shader_desc.InputParameters; ++i) {
			
			D3D12_SIGNATURE_PARAMETER_DESC param_desc;

			ThrowIfFailed(shader_reflection->GetInputParameterDesc(i, &param_desc));

			D3D12InputElementDesc element;
			
			element.SetSemanticName(param_desc.SemanticName)
				.SetSemanticIndex(param_desc.SemanticIndex)
				.SetInputSlot(slot)
				.SetAlignedByteOffset(offset)
				.SetInputSlotClass(D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA)
				.SetInstanceDataStepRate(0);

			int component_index = -1;
			switch (param_desc.ComponentType) {
			case D3D_REGISTER_COMPONENT_UINT32:  component_index = 0; break;
			case D3D_REGISTER_COMPONENT_SINT32:  component_index = 1; break;
			case D3D_REGISTER_COMPONENT_FLOAT32: component_index = 2; break;
			}

			int mask_index = -1;
			if (param_desc.Mask == 1) mask_index = 0;
			else if (param_desc.Mask <= 3) mask_index = 1;
			else if (param_desc.Mask <= 7) mask_index = 2;
			else if (param_desc.Mask <= 15) mask_index = 3;

			if (component_index != -1 && mask_index != -1) {
				element.SetFormat(FormatTable[mask_index][component_index]);
			}

			offset += AlignedOffset(element.Format);

			if (param_desc.SystemValueType == D3D_NAME_UNDEFINED) {
				input_layout.emplace_back(std::move(element));
			}

		}

		return input_layout;

	}

	static std::pair<std::vector<CD3DX12_ROOT_PARAMETER1>, std::vector<CD3DX12_STATIC_SAMPLER_DESC>> ReflectRootSignature(
		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> const& shader_reflection,
		D3D12_SHADER_VISIBILITY shader_visibility
	) {

		D3D12_SHADER_DESC shader_desc;
		ThrowIfFailed(shader_reflection->GetDesc(&shader_desc));

		std::vector<CD3DX12_ROOT_PARAMETER1> root_params;
		std::vector<CD3DX12_STATIC_SAMPLER_DESC> static_samplers;

		for (std::uint32_t i = 0; i < shader_desc.BoundResources; ++i) {

			D3D12_SHADER_INPUT_BIND_DESC bind_desc;
			if (FAILED(shader_reflection->GetResourceBindingDesc(i, &bind_desc))) {
				continue;
			}

			switch (bind_desc.Type) {
			case D3D_SIT_CBUFFER: {
				CD3DX12_ROOT_PARAMETER1 root_param;
				root_param.InitAsConstantBufferView(
					bind_desc.BindPoint,
					bind_desc.Space,
					D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
					shader_visibility
				);
				root_params.emplace_back(std::move(root_param));
				break;
			}

			case D3D_SIT_TBUFFER:
			case D3D_SIT_TEXTURE: {
				CD3DX12_ROOT_PARAMETER1 root_param;
				root_param.InitAsShaderResourceView(
					bind_desc.BindPoint,
					bind_desc.Space,
					D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
					shader_visibility
				);
				root_params.emplace_back(std::move(root_param));
				break;
			}

			case D3D_SIT_SAMPLER: {
				CD3DX12_STATIC_SAMPLER_DESC sampler_desc(
					bind_desc.BindPoint,
					D3D12_FILTER_MIN_MAG_MIP_LINEAR,
					D3D12_TEXTURE_ADDRESS_MODE_WRAP,
					D3D12_TEXTURE_ADDRESS_MODE_WRAP,
					D3D12_TEXTURE_ADDRESS_MODE_WRAP
				);
				sampler_desc.ShaderVisibility = shader_visibility;
				static_samplers.emplace_back(std::move(sampler_desc));
				break;
			}

			case D3D_SIT_UAV_RWSTRUCTURED:
			case D3D_SIT_UAV_RWTYPED: {
				CD3DX12_ROOT_PARAMETER1 root_param;
				root_param.InitAsUnorderedAccessView(
					bind_desc.BindPoint,
					bind_desc.Space,
					D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
					shader_visibility
				);
				root_params.emplace_back(std::move(root_param));
				break;
			}

			}

		}

		return std::make_pair(std::move(root_params), std::move(static_samplers));

	}

	void D3D12PipelineStateObjectBuilder::RethrowException() const {
		if (m_vertex_shader_exception) {
			std::rethrow_exception(m_vertex_shader_exception);
		}
		if(m_pixel_shader_exception) {
			std::rethrow_exception(m_pixel_shader_exception);
		}
	}

	ID3D12Device* D3D12PipelineStateObjectBuilder::LoadDevice() const noexcept {

		return std::visit(
			[](auto&& param) -> ID3D12Device* {
				using T = std::decay_t<decltype(param)>;
				if constexpr (std::is_same_v<T, ID3D12Device*>) {
					return param;
				}
				else if constexpr (std::is_same_v<T, Microsoft::WRL::ComPtr<ID3D12Device>>) {
					return param.Get();
				}
				else {
					return nullptr;
				}
			},
			m_device
		);

	}

	void D3D12PipelineStateObjectBuilder::LoadShader(
		std::filesystem::path const& path,
		std::vector<std::byte>& output, 
		std::atomic_flag& flag
	) {
		output = ReadFile(path);
		flag.test_and_set(std::memory_order::release);
	}

	D3D12PipelineStateObjectBuilder& D3D12PipelineStateObjectBuilder::LoadVertexShaderImpl(std::filesystem::path const& path) {

		s_scheduler->SubmitTask(
			[this, path]() {
				LoadShader(path, m_vertex_shader, m_vertex_shader_ready);
			}
		);

		return *this;

	}

	D3D12PipelineStateObjectBuilder& D3D12PipelineStateObjectBuilder::LoadFragmentShaderImpl(std::filesystem::path const& path) {
		return LoadPixelShader(path);
	}

	D3D12PipelineStateObjectBuilder& D3D12PipelineStateObjectBuilder::LoadPixelShaderImpl(std::filesystem::path const& path) {

		s_scheduler->SubmitTask(
			[this, path]() {
				LoadShader(path, m_pixel_shader, m_pixel_shader_ready);
			}
		);

		return *this;

	}

	void D3D12PipelineStateObjectBuilder::CompileShader(
		std::filesystem::path const& path,
		std::vector<std::byte>& output,
		std::atomic_flag& flag,
		std::string_view entry,
		std::string_view target_profile
	) {

		util::Defer defer(
			[&flag]() {
				flag.test_and_set(std::memory_order::release);
			}
		);

		try {	
			std::vector<std::byte> source = ReadFile(path);
			auto shader = ::fyuu_engine::windows::d3d12::CompileShader(
				source,
				entry,
				target_profile
			);
			output.resize(shader->GetBufferSize());
			std::memcpy(output.data(), shader->GetBufferPointer(), shader->GetBufferSize());
		}
		catch (...) {
			throw;
		}

	}

	D3D12PipelineStateObjectBuilder& D3D12PipelineStateObjectBuilder::CompileVertexShaderImpl(
		std::filesystem::path const& path,
		std::string_view entry
	) {
		
		s_scheduler->SubmitTask(
			[this, path, entry]() {
				try {
					CompileShader(path, m_vertex_shader, m_vertex_shader_ready, entry, "vs_5_0");
				}
				catch (...) {
					m_vertex_shader_exception = std::current_exception();
				}
			}
		);

		return *this;

	}

	D3D12PipelineStateObjectBuilder& D3D12PipelineStateObjectBuilder::CompileFragmentShaderImpl(
		std::filesystem::path const& path,
		std::string_view entry
	) {
		return CompilePixelShaderImpl(path, entry);
	}

	D3D12PipelineStateObjectBuilder& D3D12PipelineStateObjectBuilder::CompilePixelShaderImpl(
		std::filesystem::path const& path,
		std::string_view entry
	) {

		s_scheduler->SubmitTask(
			[this, path, entry]() {
				try {
					CompileShader(path, m_pixel_shader, m_pixel_shader_ready, entry, "ps_5_0");
				}
				catch (...) {
					m_pixel_shader_exception = std::current_exception();
				}
			}
		);

		return *this;

	}

	concurrency::AsyncTask<D3D12PipelineStateObject> D3D12PipelineStateObjectBuilder::BuildImpl() const {

		ID3D12Device* device = LoadDevice();
		if (!device) {
			throw std::runtime_error("Invalid D3D12 device");
		}

		co_await s_scheduler->WaitForConditions(
			[this]() {
				return m_vertex_shader_ready.test(std::memory_order::acquire);
			},
			[this]() {
				return m_pixel_shader_ready.test(std::memory_order::acquire);
			}
		);

		RethrowException();

		/*
		*	reflection
		*/

		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> vertex_shader_reflection;
		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> pixel_shader_reflection;
		ThrowIfFailed(D3DReflect(m_vertex_shader.data(), m_vertex_shader.size(), IID_PPV_ARGS(&vertex_shader_reflection)));
		ThrowIfFailed(D3DReflect(m_pixel_shader.data(), m_pixel_shader.size(), IID_PPV_ARGS(&pixel_shader_reflection)));

		auto [vertex_input_layout_future, vertex_root_param_future, pixel_root_param_future] =
			co_await s_scheduler->WaitForTasks(
				[this, vertex_shader_reflection]() {
					return ReflectInputLayout(vertex_shader_reflection, m_slot);
				},
				[vertex_shader_reflection]() {
					return ReflectRootSignature(vertex_shader_reflection, D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX);
				},
				[pixel_shader_reflection]() {
					return ReflectRootSignature(pixel_shader_reflection, D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL);
				}
			);

		std::vector<D3D12InputElementDesc> vertex_input_layout = vertex_input_layout_future.get();
		auto [vertex_root_params, vertex_static_samplers] = vertex_root_param_future.get();
		auto [pixel_root_params, pixel_static_samplers] = pixel_root_param_future.get();

		/*
		*	merge root parameters
		*/

		std::vector<CD3DX12_ROOT_PARAMETER1> root_parameters;
		root_parameters.reserve(vertex_root_params.size() + pixel_root_params.size());
		root_parameters.insert(root_parameters.end(), vertex_root_params.begin(), vertex_root_params.end());
		root_parameters.insert(root_parameters.end(), pixel_root_params.begin(), pixel_root_params.end());

		std::vector<CD3DX12_STATIC_SAMPLER_DESC> static_samplers;
		static_samplers.reserve(vertex_static_samplers.size() + pixel_static_samplers.size());
		static_samplers.insert(static_samplers.end(), vertex_static_samplers.begin(), vertex_static_samplers.end());
		static_samplers.insert(static_samplers.end(), pixel_static_samplers.begin(), pixel_static_samplers.end());

		/*
		*	Create root signature
		*/

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
		root_signature_desc.Init_1_1(
			static_cast<UINT>(root_parameters.size()),
			root_parameters.data(),
			static_cast<UINT>(static_samplers.size()),
			static_samplers.data(),
			m_root_signature_flags
		);

		Microsoft::WRL::ComPtr<ID3DBlob> signature_blob;
		Microsoft::WRL::ComPtr<ID3DBlob> error_blob;
		ThrowIfFailed(
			D3DX12SerializeVersionedRootSignature(
				&root_signature_desc,
				D3D_ROOT_SIGNATURE_VERSION_1_1,
				&signature_blob,
				&error_blob
			)
		);

		Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
		ThrowIfFailed(
			device->CreateRootSignature(
				0,
				signature_blob->GetBufferPointer(),
				signature_blob->GetBufferSize(),
				IID_PPV_ARGS(&root_signature)
			)
		);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc{};
		pso_desc.pRootSignature = root_signature.Get();
		pso_desc.VS = {
			m_vertex_shader.data(),
			m_vertex_shader.size()
		};
		pso_desc.PS = {
			m_pixel_shader.data(),
			m_pixel_shader.size()
		};
		pso_desc.InputLayout = {
			vertex_input_layout.data(),
			static_cast<std::uint32_t>(vertex_input_layout.size())
		};

		pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		pso_desc.DepthStencilState.DepthEnable = false;

		pso_desc.SampleMask = UINT_MAX;
		pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pso_desc.NumRenderTargets = 1;
		pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		pso_desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		pso_desc.SampleDesc.Count = 1;
		pso_desc.SampleDesc.Quality = 0;
		pso_desc.NodeMask = 0;
		pso_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;
		ThrowIfFailed(device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline_state)));

		co_return D3D12PipelineStateObject{ std::move(root_signature), std::move(pipeline_state) };

	}

	D3D12PipelineStateObjectBuilder::D3D12PipelineStateObjectBuilder()
		: Base() {
	}

	D3D12PipelineStateObjectBuilder& D3D12PipelineStateObjectBuilder::SetDevice(ID3D12Device* device) {
		m_device.emplace<ID3D12Device*>(device);
		return *this;
	}

	D3D12PipelineStateObjectBuilder& D3D12PipelineStateObjectBuilder::SetDevice(Microsoft::WRL::ComPtr<ID3D12Device> const& device) {
		m_device.emplace<Microsoft::WRL::ComPtr<ID3D12Device>>(device);
		return *this;
	}

	D3D12PipelineStateObjectBuilder& D3D12PipelineStateObjectBuilder::SetVertexInputSlot(std::uint32_t slot) {
		m_slot = slot;
		return *this;
	}

	void D3D12SPIRVPipelineStateObjectBuilder::ProcessPushConstants(
		spirv_cross::Compiler& compiler, 
		spirv_cross::ShaderResources& resources, 
		std::vector<CD3DX12_ROOT_PARAMETER>& out_root_parameters
	) const {

		auto& push_constant_buffers = resources.push_constant_buffers;
		if (push_constant_buffers.empty()) {
			return;
		}

		auto& push_constant_buffer = push_constant_buffers[0];

		spirv_cross::SPIRType const& type = compiler.get_type(push_constant_buffer.type_id);
		std::size_t size_in_bytes = compiler.get_declared_struct_size(type);
		std::uint32_t size_in_dwords = static_cast<std::uint32_t>(size_in_bytes / 4);

		std::uint32_t binding = compiler.get_decoration(push_constant_buffer.id, spv::DecorationBinding);
		std::uint32_t set = compiler.get_decoration(push_constant_buffer.id, spv::DecorationDescriptorSet);

		CD3DX12_ROOT_PARAMETER root_param = {};
		root_param.InitAsConstants(size_in_dwords, binding, set, D3D12_SHADER_VISIBILITY_ALL);

		out_root_parameters.emplace_back(std::move(root_param));

	}

	void D3D12SPIRVPipelineStateObjectBuilder::ProcessResources(
		spirv_cross::Compiler& compiler,
		std::span<spirv_cross::Resource> resources,
		D3D12_DESCRIPTOR_RANGE_TYPE range_type,
		std::vector<CD3DX12_ROOT_PARAMETER>& out_root_parameters,
		std::vector<CD3DX12_DESCRIPTOR_RANGE>& out_descriptor_ranges,
		D3D12_SHADER_VISIBILITY visibility
	) const {

		for (auto const& resource : resources) {
			std::uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			std::uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

			/*
			*	create descriptor range
			*/

			CD3DX12_DESCRIPTOR_RANGE descriptor_range = {};
			descriptor_range.Init(
				range_type,
				1, // num descriptors
				binding,
				set
			);
			auto& range = out_descriptor_ranges.emplace_back(std::move(descriptor_range));

			/*
			*	create root parameter
			*/

			CD3DX12_ROOT_PARAMETER root_param = {};
			root_param.InitAsDescriptorTable(
				1, // num descriptor ranges
				&range,
				visibility
			);
			out_root_parameters.emplace_back(std::move(root_param));
		}

	}

	void D3D12SPIRVPipelineStateObjectBuilder::ProcessConstantBuffers(
		spirv_cross::Compiler& compiler, 
		spirv_cross::ShaderResources& resources, 
		std::vector<CD3DX12_ROOT_PARAMETER>& out_root_parameters, 
		std::vector<CD3DX12_DESCRIPTOR_RANGE>& out_descriptor_ranges,
		D3D12_SHADER_VISIBILITY visibility
	) const {

		ProcessResources(
			compiler,
			resources.uniform_buffers,
			D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
			out_root_parameters,
			out_descriptor_ranges,
			visibility
		);

	}

	void D3D12SPIRVPipelineStateObjectBuilder::ProcessTexture(
		spirv_cross::Compiler& compiler,
		spirv_cross::ShaderResources& resources,
		std::vector<CD3DX12_ROOT_PARAMETER>& out_root_parameters,
		std::vector<CD3DX12_DESCRIPTOR_RANGE>& out_descriptor_ranges,
		D3D12_SHADER_VISIBILITY visibility
	) const {

		ProcessResources(
			compiler,
			resources.sampled_images,
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
			out_root_parameters,
			out_descriptor_ranges,
			visibility
		);

	}

	void D3D12SPIRVPipelineStateObjectBuilder::ProcessSampler(
		spirv_cross::Compiler& compiler, 
		spirv_cross::ShaderResources& resources, 
		std::vector<CD3DX12_ROOT_PARAMETER>& out_root_parameters, 
		std::vector<CD3DX12_DESCRIPTOR_RANGE>& out_descriptor_ranges, 
		D3D12_SHADER_VISIBILITY visibility
	) const {

		ProcessResources(
			compiler,
			resources.separate_samplers,
			D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
			out_root_parameters,
			out_descriptor_ranges,
			visibility
		);

	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> D3D12SPIRVPipelineStateObjectBuilder::CreateRootSignature(
		spirv_cross::Compiler& vertex_compiler,
		spirv_cross::ShaderResources& vertex_resource,
		spirv_cross::Compiler& fragment_compiler,
		spirv_cross::ShaderResources& fragment_resource
	) const {

		std::vector<CD3DX12_ROOT_PARAMETER> root_parameters;
		std::vector<CD3DX12_DESCRIPTOR_RANGE> descriptor_ranges;

		std::size_t resource_count = 
			vertex_resource.push_constant_buffers.size() +
			vertex_resource.uniform_buffers.size() +
			vertex_resource.sampled_images.size() +
			vertex_resource.separate_samplers.size() +
			fragment_resource.push_constant_buffers.size() +
			fragment_resource.uniform_buffers.size() +
			fragment_resource.sampled_images.size() +
			fragment_resource.separate_samplers.size();

		root_parameters.reserve(resource_count);
		descriptor_ranges.reserve(resource_count);

		/*
		*	Process resources
		*/

		ProcessPushConstants(vertex_compiler, vertex_resource, root_parameters);

		ProcessConstantBuffers(
			vertex_compiler, vertex_resource, 
			root_parameters, descriptor_ranges, 
			D3D12_SHADER_VISIBILITY_VERTEX
		);

		ProcessConstantBuffers(
			fragment_compiler, fragment_resource, 
			root_parameters, descriptor_ranges, 
			D3D12_SHADER_VISIBILITY_PIXEL
		);

		ProcessTexture(
			vertex_compiler, vertex_resource,
			root_parameters, descriptor_ranges,
			D3D12_SHADER_VISIBILITY_VERTEX
		);

		ProcessTexture(
			fragment_compiler, fragment_resource,
			root_parameters, descriptor_ranges,
			D3D12_SHADER_VISIBILITY_PIXEL
		);

		/*
		*	create root signature
		*/

		CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc = {};
		root_signature_desc.Init(
			static_cast<std::uint32_t>(root_parameters.size()),
			root_parameters.data(),
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		/*
		*	serialize root signature
		*/

		Microsoft::WRL::ComPtr<ID3DBlob> serialized_root_signature;
		Microsoft::WRL::ComPtr<ID3DBlob> error_blob;
		
		ThrowIfFailed(D3D12SerializeRootSignature(
			&root_signature_desc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&serialized_root_signature,
			&error_blob
		));

		Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;

		ID3D12Device* device = std::visit(
			[](auto&& param) -> ID3D12Device* {
				using T = std::decay_t<decltype(param)>;
				if constexpr (std::is_same_v<T, ID3D12Device*>) {
					return param;
				}
				else if constexpr (std::is_same_v<T, Microsoft::WRL::ComPtr<ID3D12Device>>) {
					return param.Get();
				}
				else {
					throw std::runtime_error("D3D12SPIRVPipelineStateObjectBuilder::CreateRootSignature: device is not set");
				}
			},
			m_device
		);

		ThrowIfFailed(device->CreateRootSignature(
			0,
			serialized_root_signature->GetBufferPointer(),
			serialized_root_signature->GetBufferSize(),
			IID_PPV_ARGS(&root_signature)
		));

		return root_signature;

	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> D3D12SPIRVPipelineStateObjectBuilder::CreateVertexInputLayout(
		spirv_cross::Compiler& vertex_compiler, 
		spirv_cross::ShaderResources& vertex_resource
	) const {

		std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout_descs;
		input_layout_descs.reserve(vertex_resource.stage_inputs.size());
		std::uint32_t aligned_offset = 0;

		for (auto& input : vertex_resource.stage_inputs) {

			spirv_cross::SPIRType const& input_type = vertex_compiler.get_type(input.type_id);

			std::string_view semantic_name = "TEXCOORD";

			if (vertex_compiler.has_decoration(input.id, spv::DecorationHlslSemanticGOOGLE)) {
				vertex_compiler.get_decoration_string(input.id, spv::DecorationHlslSemanticGOOGLE);
			}

			std::uint32_t semantic_index = vertex_compiler.has_decoration(input.id, spv::DecorationLocation) ?
				vertex_compiler.get_decoration(input.id, spv::DecorationLocation) :
				0;

			DXGI_FORMAT dxgi_format = SPIRTypeToDXGIFormat(input_type);

			D3D12_INPUT_ELEMENT_DESC desc{};

			desc.SemanticName = semantic_name.data();
			desc.SemanticIndex = semantic_index;
			desc.Format = SPIRTypeToDXGIFormat(input_type);
			desc.InputSlot = m_vertex_input_slot;
			desc.AlignedByteOffset = aligned_offset;
			desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			desc.InstanceDataStepRate = 0;

			input_layout_descs.emplace_back(std::move(desc));

			aligned_offset += AlignedOffset(dxgi_format);

		}

		return input_layout_descs;
	}

	Microsoft::WRL::ComPtr<ID3DBlob> D3D12SPIRVPipelineStateObjectBuilder::CompileShader(
		std::span<std::uint32_t const> spirv_byte_code,
		std::string_view entry_point, 
		std::string_view target_profile
	) const {
		spirv_cross::CompilerHLSL::Options options;
		options.shader_model = 50;
		spirv_cross::CompilerHLSL compiler(spirv_byte_code.data(), spirv_byte_code.size());
		compiler.set_hlsl_options(options);
		std::string hlsl_code = compiler.compile();

		UINT compile_flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
		compile_flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG
		Microsoft::WRL::ComPtr<ID3DBlob> shader_blob;
		Microsoft::WRL::ComPtr<ID3DBlob> error_blob;
		HRESULT result = D3DCompile(
			hlsl_code.data(),
			hlsl_code.size(),
			nullptr,
			nullptr,
			nullptr,
			entry_point.data(),
			target_profile.data(),
			compile_flags,
			0,
			&shader_blob,
			&error_blob
		);

		if (FAILED(result)) {
			if (error_blob) {
				std::string error_message(static_cast<char const*>(error_blob->GetBufferPointer()), error_blob->GetBufferSize());
				throw std::runtime_error("D3DCompile failed: " + error_message);
			}
			else {
				_com_error err(result);
				throw std::runtime_error("D3DCompile failed: " + std::string(err.ErrorMessage()));
			}
		}

		return shader_blob;
	}

	concurrency::AsyncTask<D3D12PipelineStateObject> D3D12SPIRVPipelineStateObjectBuilder::BuildImpl() const {

		spirv_cross::Compiler vertex_compiler(m_vertex_shader_byte_code);
		spirv_cross::Compiler fragment_compiler(m_fragment_shader_byte_code);

		spirv_cross::ShaderResources vertex_resources = vertex_compiler.get_shader_resources();
		spirv_cross::ShaderResources fragment_resources = fragment_compiler.get_shader_resources();

		/*
		*	Dispatch tasks
		*/

		auto [vertex_shader_future, pixel_shader_future, root_signature_future, input_layout_future]
			= co_await s_scheduler->WaitForTasks(
				[this]() {
					return CompileShader(m_vertex_shader_byte_code, "main", "vs_5_0");
				},
				[this]() {
					return CompileShader(m_fragment_shader_byte_code, "main", "ps_5_0");
				},
				[this, &vertex_compiler, &vertex_resources, &fragment_compiler, &fragment_resources]() {
					return CreateRootSignature(
						vertex_compiler,
						vertex_resources,
						fragment_compiler,
						fragment_resources
					);
				},
				[this, &vertex_compiler, &vertex_resources]() {
					return CreateVertexInputLayout(
						vertex_compiler,
						vertex_resources
					);
				}
			);

		Microsoft::WRL::ComPtr<ID3DBlob> vertex_shader = vertex_shader_future.get();
		Microsoft::WRL::ComPtr<ID3DBlob> pixel_shader = pixel_shader_future.get();
		Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature = root_signature_future.get();
		std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout = input_layout_future.get();

		D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};

		pso_desc.pRootSignature = root_signature.Get();
		pso_desc.VS = { vertex_shader->GetBufferPointer(), vertex_shader->GetBufferSize() };
		pso_desc.PS = { pixel_shader->GetBufferPointer(), pixel_shader->GetBufferSize() };
		pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		pso_desc.SampleMask = UINT_MAX;
		pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		pso_desc.InputLayout = { input_layout.data(), static_cast<UINT>(input_layout.size()) };
		pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pso_desc.NumRenderTargets = 1;
		pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		pso_desc.SampleDesc.Count = 1;
		pso_desc.SampleDesc.Quality = 0;
		pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		pso_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		pso_desc.NodeMask = 0;
		pso_desc.CachedPSO = { nullptr, 0 };
		pso_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		pso_desc.StreamOutput = { nullptr, 0, 0 };
		pso_desc.DepthStencilState.DepthEnable = FALSE;
		pso_desc.DepthStencilState.StencilEnable = FALSE;
		pso_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		pso_desc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		pso_desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		pso_desc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		pso_desc.RasterizerState.ForcedSampleCount = 0;
		pso_desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;
		ID3D12Device* device = std::visit(
			[](auto&& param) -> ID3D12Device* {
				using T = std::decay_t<decltype(param)>;
				if constexpr (std::is_same_v<T, ID3D12Device*>) {
					return param;
				}
				else if constexpr (std::is_same_v<T, Microsoft::WRL::ComPtr<ID3D12Device>>) {
					return param.Get();
				}
				else {
					throw std::runtime_error("D3D12SPIRVPipelineStateObjectBuilder::BuildImpl: device is not set");
				}
			},
			m_device
		);

		ThrowIfFailed(device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline_state)));

		co_return D3D12PipelineStateObject{ root_signature,pipeline_state };

	}

	std::vector<std::uint32_t> D3D12SPIRVPipelineStateObjectBuilder::ReadFile(std::filesystem::path const& path) {

		std::size_t size_in_bytes = fs::file_size(path);

		std::ifstream file(path, std::ios_base::binary);
		std::vector<std::uint32_t> byte_code(size_in_bytes / 4, 0u);
		file.read(reinterpret_cast<char*>(byte_code.data()), size_in_bytes);

		return byte_code;

	}

	D3D12SPIRVPipelineStateObjectBuilder& D3D12SPIRVPipelineStateObjectBuilder::LoadVertexShaderImpl(std::filesystem::path const& path) {
		m_vertex_shader_byte_code = D3D12SPIRVPipelineStateObjectBuilder::ReadFile(path);
		return *this;
	}

	D3D12SPIRVPipelineStateObjectBuilder& D3D12SPIRVPipelineStateObjectBuilder::LoadFragmentShaderImpl(std::filesystem::path const& path) {
		m_fragment_shader_byte_code = D3D12SPIRVPipelineStateObjectBuilder::ReadFile(path);
		return *this;
	}

	D3D12SPIRVPipelineStateObjectBuilder::D3D12SPIRVPipelineStateObjectBuilder()
		: Base(),
		m_vertex_input_slot(0u) {

	}

	D3D12SPIRVPipelineStateObjectBuilder& D3D12SPIRVPipelineStateObjectBuilder::SetDevice(ID3D12Device* device) {
		m_device.emplace<ID3D12Device*>(device);
		return *this;
	}

	D3D12SPIRVPipelineStateObjectBuilder& D3D12SPIRVPipelineStateObjectBuilder::SetDevice(Microsoft::WRL::ComPtr<ID3D12Device> const& device) {
		m_device.emplace<Microsoft::WRL::ComPtr<ID3D12Device>>(device);
		return *this;
	}

	D3D12SPIRVPipelineStateObjectBuilder& D3D12SPIRVPipelineStateObjectBuilder::SetVertexInputSlot(std::uint32_t slot) {
		m_vertex_input_slot = slot;
		return *this;
	}

#endif // WIN32

}