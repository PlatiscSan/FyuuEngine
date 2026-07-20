module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <string_view>
#include <concepts>
#include <vector>
#include <cstdint>
#include <span>
#endif // !defined(__cpp_lib_modules)

export module fyuu_rhi:logical_device;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :resource_types;
import :resource;
import :view;
import :sampler_types;
import :sampler;
import :pipeline_types;
import :pipeline;
import :native_pipeline_binding;

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

		Resource<Backend> CreateBuffer(std::size_t size_in_bytes, ResourceFlags const& flags) {
			using Ret = decltype(Backend::CreateBuffer(m_impl, size_in_bytes, flags));
			static_assert(std::constructible_from<Resource<Backend>, Ret>,
				"Resource<Backend> must be constructible from buffer returned by CreateBuffer()");
			return Backend::CreateBuffer(m_impl, size_in_bytes, flags);
		}

		Resource<Backend> CreateTexture(std::size_t width, std::size_t height, std::size_t depth_arr_layers, std::size_t mip_lvl_cnt, ResourceFlags const& flags) {
			using Ret = decltype(Backend::CreateTexture(m_impl, width, height, depth_arr_layers, mip_lvl_cnt, flags));
			static_assert(std::constructible_from<Resource<Backend>, Ret>,
				"Resource<Backend> must be constructible from texture returned by CreateTexture()");
			return Backend::CreateTexture(m_impl, width, height, depth_arr_layers, mip_lvl_cnt, flags);
		}

		View<Backend> CreateBufferView(Resource<Backend> const& buf, std::size_t offset, std::size_t range, ResourceFlags const& flags) {
			using Ret = decltype(Backend::CreateBufferView(m_impl, buf.GetLogicalDevicePassKey().GetImplementation(), offset, range, flags));
			static_assert(std::constructible_from<View<Backend>, Ret>,
				"View<Backend> must be constructible from view returned by CreateBufferView()");
			return Backend::CreateBufferView(m_impl, buf.GetLogicalDevicePassKey().GetImplementation(), offset, range, flags);
		}

		View<Backend> CreateTextureView(Resource<Backend> const& tex, std::size_t base_mip_lvl, std::size_t mip_lvl_cnt, std::size_t base_arr_layer, std::size_t arr_layer_cnt, ResourceFlags const& flags) {
			using Ret = decltype(Backend::CreateTextureView(m_impl, tex.GetLogicalDevicePassKey().GetImplementation(), base_mip_lvl, mip_lvl_cnt, base_arr_layer, arr_layer_cnt, flags));
			static_assert(std::constructible_from<View<Backend>, Ret>,
				"View<Backend> must be constructible from view returned by CreateTextureView()");
			return Backend::CreateTextureView(m_impl, tex.GetLogicalDevicePassKey().GetImplementation(), base_mip_lvl, mip_lvl_cnt, base_arr_layer, arr_layer_cnt, flags);
		}

		Sampler<Backend> CreateSampler(SamplerDescriptor const& descriptor) {
			using Ret = decltype(Backend::CreateSampler(m_impl, descriptor));
			static_assert(std::constructible_from<Sampler<Backend>, Ret>,
				"Sampler<Backend> must be constructible from sampler returned by CreateSampler()");
			return Backend::CreateSampler(m_impl, descriptor);
		}

		Pipeline<Backend> CreateGraphicsPipeline(GraphicsPipelineDescriptor const& descriptor) {
			using Ret = decltype(Backend::CreateGraphicsPipeline(m_impl, descriptor));
			static_assert(
				std::constructible_from<Pipeline<Backend>, Ret>,
				"Pipeline<Backend> must be constructible from pipeline returned by CreateGraphicsPipeline()"
			);
			return Backend::CreateGraphicsPipeline(m_impl, descriptor);
		}

		PipelineResourceGroup<Backend> CreatePipelineResourceGroup(
			Pipeline<Backend> const& pipeline,
			std::uint32_t space,
			std::span<PipelineResourceBinding<Backend> const> bindings
		) {
			std::vector<NativePipelineResourceBinding<Backend>> native_bindings;
			native_bindings.reserve(bindings.size());
			for (auto const& binding : bindings) {
				auto const& value = binding.value;
				auto buffer = value.Buffer();
				auto view = value.BoundView();
				auto sampler = value.BoundSampler();
				NativePipelineBindingValue<Backend> native_value;
				if (buffer) {
					native_value = NativePipelineBufferBinding<Backend>{
						.buffer = buffer->GetLogicalDevicePassKey().GetImplementation(),
						.offset = value.Offset(),
						.size = value.Size()
					};
				}
				else if (view && sampler) {
					native_value = NativePipelineCombinedBinding<Backend>{
						.view = view->GetPassKey().GetImplementation(),
						.sampler = sampler->GetPassKey().GetImplementation()
					};
				}
				else if (view) {
					native_value = NativePipelineViewBinding<Backend>{
						.view = view->GetPassKey().GetImplementation()
					};
				}
				else if (sampler) {
					native_value = NativePipelineSamplerBinding<Backend>{
						.sampler = sampler->GetPassKey().GetImplementation()
					};
				}
				native_bindings.push_back(
					{
						.binding = binding.binding,
						.array_element = binding.array_element,
						.value = std::move(native_value)
					}
				);
			}
			auto impl = Backend::CreatePipelineResourceGroup(
				m_impl,
				pipeline.GetPassKey().GetImplementation(),
				space,
				native_bindings
			);
			return PipelineResourceGroup<Backend>(std::move(impl), space);
		}

	};

}
