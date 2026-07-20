module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>
#endif // !defined(__cpp_lib_modules)

export module fyuu_rhi:pipeline;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :pipeline_types;
import :resource;
import :view;
import :sampler;

namespace fyuu_rhi {

	export template <class Backend> class Pipeline {
	public:
		using Implementation = typename Backend::Pipeline;

	private:
		Implementation m_impl;

	public:
		template <class PipelineType>
		class PassKey {
			static_assert(
				std::same_as<PipelineType, Pipeline> || std::same_as<PipelineType, Pipeline const>,
				"PipelineType must be Pipeline or const Pipeline"
			);

			template <class U> friend class LogicalDevice;
			template <class U> friend class PipelineResourceGroup;

			PipelineType* m_pipeline;

			template <class Self>
			decltype(auto) GetImplementation(this Self&& self) noexcept {
				return self.m_pipeline->m_impl;
			}

		public:
			explicit PassKey(PipelineType* pipeline) noexcept
				: m_pipeline(pipeline) {

			}
		};

		template <std::convertible_to<Implementation> I>
		Pipeline(I&& impl)
			: m_impl(std::forward<I>(impl)) {

		}

		template <class Self>
		auto GetPassKey(this Self&& self) noexcept {
			using PipelineType = std::remove_reference_t<Self>;
			return PassKey<PipelineType>{ &self };
		}
	};

	export template <class Backend> class PipelineBindingValue {
	private:
		Resource<Backend> const* m_buffer = nullptr;
		View<Backend> const* m_view = nullptr;
		Sampler<Backend> const* m_sampler = nullptr;
		std::size_t m_offset = 0;
		std::size_t m_size = PipelineWholeBuffer;

		template <class U> friend class LogicalDevice;

		PipelineBindingValue(
			Resource<Backend> const* buffer,
			View<Backend> const* view,
			Sampler<Backend> const* sampler,
			std::size_t offset,
			std::size_t size
		) noexcept
			: m_buffer(buffer),
			m_view(view),
			m_sampler(sampler),
			m_offset(offset),
			m_size(size) {

		}

	public:
		static PipelineBindingValue FromBuffer(
			Resource<Backend> const& buffer,
			std::size_t offset = 0,
			std::size_t size = PipelineWholeBuffer
		) noexcept {
			return { &buffer, nullptr, nullptr, offset, size };
		}

		static PipelineBindingValue FromView(View<Backend> const& view) noexcept {
			return { nullptr, &view, nullptr, 0, PipelineWholeBuffer };
		}

		static PipelineBindingValue FromSampler(Sampler<Backend> const& sampler) noexcept {
			return { nullptr, nullptr, &sampler, 0, PipelineWholeBuffer };
		}

		static PipelineBindingValue FromCombined(
			View<Backend> const& view,
			Sampler<Backend> const& sampler
		) noexcept {
			return { nullptr, &view, &sampler, 0, PipelineWholeBuffer };
		}

		Resource<Backend> const* Buffer() const noexcept {
			return m_buffer;
		}

		View<Backend> const* BoundView() const noexcept {
			return m_view;
		}

		Sampler<Backend> const* BoundSampler() const noexcept {
			return m_sampler;
		}

		std::size_t Offset() const noexcept {
			return m_offset;
		}

		std::size_t Size() const noexcept {
			return m_size;
		}
	};

	export template <class Backend> struct PipelineResourceBinding {
		std::uint32_t binding = 0;
		std::uint32_t array_element = 0;
		PipelineBindingValue<Backend> value;
	};

	// A resource group is created for one pipeline space and is immutable after
	// creation. The backend retains the resources needed to materialize a
	// Vulkan descriptor set, WebGPU bind group, D3D12 descriptor tables, or
	// OpenGL bindings when command encoding is introduced.
	export template <class Backend> class PipelineResourceGroup {
	public:
		using Implementation = typename Backend::PipelineResourceGroup;

	private:
		Implementation m_impl;
		std::uint32_t m_space = 0;

	public:
		template <std::convertible_to<Implementation> I>
		PipelineResourceGroup(I&& impl, std::uint32_t space)
			: m_impl(std::forward<I>(impl)),
			m_space(space) {

		}

		std::uint32_t Space() const noexcept {
			return m_space;
		}
	};

}
