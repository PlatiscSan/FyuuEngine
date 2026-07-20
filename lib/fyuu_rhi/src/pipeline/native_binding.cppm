module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <vector>
#endif // !defined(__cpp_lib_modules)

module fyuu_rhi:native_pipeline_binding;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :pipeline_types;
import :resource_types;
import :slang_pipeline_interface;

namespace fyuu_rhi {

	template <class Backend>
	struct NativePipelineResourceBinding {
		std::uint32_t binding = 0;
		std::uint32_t array_element = 0;
		typename Backend::Resource const* buffer = nullptr;
		typename Backend::View const* view = nullptr;
		typename Backend::Sampler const* sampler = nullptr;
		std::size_t offset = 0;
		std::size_t size = PipelineWholeBuffer;
	};

	struct PipelineBindingMetadata {
		ResourceFlags flags;
		std::uint32_t binding = 0;
		std::uint32_t space = 0;
		std::uint32_t count = 1;
	};

	std::vector<PipelineBindingMetadata> MakePipelineBindingMetadata(
		SlangPipelineInterface const& pipeline_interface
	) {
		std::vector<PipelineBindingMetadata> result;
		result.reserve(pipeline_interface.bindings.size());
		for (auto const& binding : pipeline_interface.bindings) {
			result.push_back(
				{
					.flags = binding.flags,
					.binding = binding.binding,
					.space = binding.space,
					.count = binding.count
				}
			);
		}
		return result;
	}

	template <class Backend>
	struct NativePipelineResourceGroup {
		struct Binding {
			std::uint32_t binding = 0;
			std::uint32_t array_element = 0;
			std::optional<typename Backend::Resource> buffer;
			std::optional<typename Backend::View> view;
			std::optional<typename Backend::Sampler> sampler;
			std::size_t offset = 0;
			std::size_t size = PipelineWholeBuffer;
		};

		std::vector<Binding> bindings;
		std::vector<PipelineBindingMetadata> layout;
	};

	template <class Backend>
	NativePipelineResourceGroup<Backend> MakePipelineResourceGroup(
		std::span<PipelineBindingMetadata const> metadata,
		std::uint32_t space,
		std::span<NativePipelineResourceBinding<Backend> const> bindings
	) {
		NativePipelineResourceGroup<Backend> result;
		for (auto const& reflected : metadata) {
			if (reflected.space == space) {
				result.layout.push_back(reflected);
			}
		}
		result.bindings.reserve(bindings.size());
		for (auto const& binding : bindings) {
			auto MatchesLocation = [space, &binding](PipelineBindingMetadata const& value) {
				return value.space == space && value.binding == binding.binding;
			};
			auto reflected = std::ranges::find_if(metadata, MatchesLocation);
			if (reflected == metadata.end()) {
				throw std::invalid_argument(
					std::format("Pipeline space {} has no binding {}", space, binding.binding)
				);
			}
			if (binding.array_element >= reflected->count) {
				throw std::out_of_range(
					std::format(
						"Pipeline binding {} array element {} exceeds count {}",
						binding.binding,
						binding.array_element,
						reflected->count
					)
				);
			}
			auto MatchesExisting = [&binding](typename NativePipelineResourceGroup<Backend>::Binding const& value) {
				return value.binding == binding.binding && value.array_element == binding.array_element;
			};
			if (std::ranges::find_if(result.bindings, MatchesExisting) != result.bindings.end()) {
				throw std::invalid_argument(
					std::format(
						"Pipeline binding {} array element {} is specified more than once",
						binding.binding,
						binding.array_element
					)
				);
			}

			bool expects_buffer =
				reflected->flags.Test(ResourceFlagBits::UniformBuffer) ||
				reflected->flags.Test(ResourceFlagBits::StorageBuffer);
			bool expects_sampler = reflected->flags.Test(ResourceFlagBits::SamplerBinding);
			bool expects_view =
				reflected->flags.Test(ResourceFlagBits::TextureBinding) ||
				reflected->flags.Test(ResourceFlagBits::StorageBinding);
			if (
				(expects_buffer != (binding.buffer != nullptr)) ||
				(expects_view != (binding.view != nullptr)) ||
				(expects_sampler != (binding.sampler != nullptr))
			) {
				throw std::invalid_argument(
					std::format("Resources do not match pipeline binding {}", binding.binding)
				);
			}
			if (!expects_buffer && (binding.offset != 0 || binding.size != PipelineWholeBuffer)) {
				throw std::invalid_argument("Only buffer bindings can specify an offset or size");
			}
			result.bindings.push_back(
				{
					.binding = binding.binding,
					.array_element = binding.array_element,
					.buffer = binding.buffer ? std::optional(*binding.buffer) : std::nullopt,
					.view = binding.view ? std::optional(*binding.view) : std::nullopt,
					.sampler = binding.sampler ? std::optional(*binding.sampler) : std::nullopt,
					.offset = binding.offset,
					.size = binding.size
				}
			);
		}
		for (auto const& reflected : metadata) {
			if (reflected.space != space) {
				continue;
			}
			for (std::uint32_t array_element = 0; array_element < reflected.count; ++array_element) {
				auto MatchesRequired = [&reflected, array_element](
					typename NativePipelineResourceGroup<Backend>::Binding const& value
				) {
					return value.binding == reflected.binding && value.array_element == array_element;
				};
				if (std::ranges::find_if(result.bindings, MatchesRequired) == result.bindings.end()) {
					throw std::invalid_argument(
						std::format(
							"Pipeline binding {} array element {} is missing",
							reflected.binding,
							array_element
						)
					);
				}
			}
		}
		return result;
	}

}
