module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <concepts>
#endif // !defined(__cpp_lib_modules)

export module fyuu_rhi:logical_device;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :resource_types;
import :command_types;
import :future;
import :resource;

namespace fyuu_rhi {

	export template <class Backend> class LogicalDevice {
	public:
		using Implementation = typename Backend::LogicalDevice;

	private:
		Implementation m_impl;

	public:
		template <std::convertible_to<Implementation> I>
		LogicalDevice(I&& impl)
			: m_impl(std::forward<I>(impl)) {

		}

		Promise<Backend> CreatePromise() const {
			using Ret = decltype(Backend::CreatePromise(m_impl));
			static_assert(std::constructible_from<Promise<Backend>, Ret>,
				"Promise<Backend> must be constructible from promise returned by CreatePromise()");
			return Backend::CreatePromise(m_impl);
		}

		Resource<Backend> CreateBuffer(std::size_t size_in_bytes, ResourceFlags const& flags) const {
			using Ret = decltype(Backend::CreateBuffer(m_impl, size_in_bytes, flags));
			static_assert(std::constructible_from<Resource<Backend>, Ret>,
				"Resource<Backend> must be constructible from buffer returned by CreateBuffer()");
			return Backend::CreateBuffer(m_impl, size_in_bytes, flags);
		}

		Resource<Backend> CreateTexture(std::size_t width, std::size_t height, std::size_t depth_arr_layers, std::size_t mip_lvl_cnt, ResourceFlags const& flags) const {
			using Ret = decltype(Backend::CreateTexture(m_impl, width, height, depth_arr_layers, mip_lvl_cnt, flags));
			static_assert(std::constructible_from<Resource<Backend>, Ret>,
				"Resource<Backend> must be constructible from texture returned by CreateTexture()");
			return Backend::CreateTexture(m_impl, width, height, depth_arr_layers, mip_lvl_cnt, flags);
		}

		Resource<Backend> CreateBufferView(Resource<Backend> const& buf, std::size_t offset, std::size_t range, ResourceFlags const& flags) const {
			using Ret = decltype(Backend::CreateBufferView(m_impl, buf.GetLogicalDevicePassKey().GetImplementation(), offset, range, flags));
			static_assert(std::constructible_from<Resource<Backend>, Ret>,
				"Resource<Backend> must be constructible from view returned by CreateBufferView()");
			return Backend::CreateBufferView(m_impl, buf.GetLogicalDevicePassKey().GetImplementation(), offset, range, flags);
		}

		Resource<Backend> CreateTextureView(Resource<Backend> const& tex, std::size_t base_mip_lvl, std::size_t mip_lvl_cnt, std::size_t base_arr_layer, std::size_t arr_layer_cnt, ResourceFlags const& flags) const {
			using Ret = decltype(Backend::CreateTextureView(m_impl, tex.GetLogicalDevicePassKey().GetImplementation(), base_mip_lvl, mip_lvl_cnt, base_arr_layer, arr_layer_cnt, flags));
			static_assert(std::constructible_from<Resource<Backend>, Ret>,
				"Resource<Backend> must be constructible from view returned by CreateTextureView()");
			return Backend::CreateTextureView(m_impl, tex.GetLogicalDevicePassKey().GetImplementation(), base_mip_lvl, mip_lvl_cnt, base_arr_layer, arr_layer_cnt, flags);
		}

		Sampler<Backend> CreateSampler(SamplerFlags const& flags) {
			using Ret = decltype(Backend::CreateSampler(m_impl, flags));
			static_assert(std::constructible_from<Sampler<Backend>, Ret>,
				"Sampler<Backend> must be constructible from sampler returned by CreateSampler()");
			return Backend::CreateSampler(m_impl, flags);
		}

		Shader<Backend> CreateShader(ShaderLanguage lang, std::string_view src) {
			using Ret = decltype(Backend::CreateShader(m_impl, lang, src));
			static_assert(std::constructible_from<Resource<Backend>, Ret>,
				"Shader<Backend> must be constructible from shader returned by CreateShader()");
			return Backend::CreateShader(m_impl, lang, src);
		}

		Shader<Backend> CreateShader(std::span<std::byte const> bytes) {
			using Ret = decltype(Backend::CreateShader(m_impl, bytes));
			static_assert(std::constructible_from<Shader<Backend>, Ret>,
				"Shader<Backend> must be constructible from shader returned by CreateShader()");
			return Backend::CreateShader(m_impl, bytes);
		}
		
		Shader<Backend> CreateShader(std::span<std::byte const> bytes) {
			using Ret = decltype(Backend::CreateShader(m_impl, bytes));
			static_assert(std::constructible_from<Shader<Backend>, Ret>,
				"Shader<Backend> must be constructible from shader returned by CreateShader()");
			return Backend::CreateShader(m_impl, bytes);
		}

		ShaderDataSegment<Backend> CreateShaderDataSegment() {
			using Ret = decltype(Backend::CreateShaderDataSegment(m_impl));
			static_assert(std::constructible_from<ShaderDataSegment<Backend>, Ret>,
				"ShaderDataSegment<Backend> must be constructible from shader data segment returned by CreateShaderDataSegment()");
			return Backend::CreateShaderDataSegment(m_impl);
		}

		GPUExecutable<Backend> CreateGPUExecutable() {
			using Ret = decltype(Backend::CreateGPUExecutable(m_impl));
			static_assert(std::constructible_from<GPUExecutable<Backend>, Ret>,
				"GPUExecutable<Backend> must be constructible from GPU executable returned by CreateGPUExecutable()");
			return Backend::CreateGPUExecutable(m_impl);
		}

		CommandBuffer<Backend> CreateCommandBuffer(CommandObjectType type) {
			using Ret = decltype(Backend::CreateCommandBuffer(m_impl, type));
			static_assert(std::constructible_from<CommandBuffer<Backend>, Ret>,
				"CommandBuffer<Backend> must be constructible from command buffer returned by CreateCommandBuffer()");
			return Backend::CreateCommandBuffer(m_impl, type);
		}

		CommandQueue<Backend> CreateCommandQueue(CommandObjectType type) {
			using Ret = decltype(Backend::CreateCommandQueue(m_impl, type));
			static_assert(std::constructible_from<CommandBuffer<Backend>, Ret>,
				"CommandQueue<Backend> must be constructible from command queue returned by CreateCommandQueue()");
			return Backend::CreateCommandQueue(m_impl, type);
		}

	};

}