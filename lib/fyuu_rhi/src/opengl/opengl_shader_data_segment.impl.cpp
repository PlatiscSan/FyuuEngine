// opengl_shader_data_segment.impl.cppm
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <utility>
#include <vector>
#include <variant>
#include <optional>
#include <unordered_map>
#endif // !defined(__cpp_lib_modules)
#include "glad.hpp"
module fyuu_rhi:opengl_shader_data_segment_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif
import :opengl_shader_data_segment;
import :types;
import :opengl_common;
import :opengl_logical_device;
import :opengl_sampler;

namespace fyuu_rhi::opengl {

	OpenGLShaderDataSegment::OpenGLShaderDataSegment(OpenGLLogicalDevice const& logical_device)
		: PolymorphicShaderDataSegmentBase(this),
		  OpenGLCommon(this, logical_device),
		  m_logical_device_id(logical_device.GetID()),
		  m_handle(std::vector<Declaration>()) {}

#define VALIDATE_STATE_AND_STAGE \
		auto* declarations = std::get_if<std::vector<Declaration>>(&m_handle); \
		if (!declarations) { \
			throw std::logic_error("Cannot declare after instantiation"); \
		} \
		if (visible == ShaderStage::Unknown) { \
			throw std::invalid_argument("Visible stage cannot be Unknown"); \
		}

	OpenGLShaderDataSegment& OpenGLShaderDataSegment::Declare(
		std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns) {
		VALIDATE_STATE_AND_STAGE
		declarations->emplace_back(where, Declaration::PushConstant{ count }, visible, ns);
		return *this;
	}

	OpenGLShaderDataSegment& OpenGLShaderDataSegment::Declare(
		ResourceFlags flags, std::size_t where, ShaderStage visible, std::size_t ns) {
		VALIDATE_STATE_AND_STAGE
		if (HasConflictingFlags(flags, ResourceFlags::Buffer, ResourceFlags::Texture)) {
			throw std::invalid_argument("Invalid or ambiguous ResourceFlags");
		}
		declarations->emplace_back(where, Declaration::Binding{ flags, 1u }, visible, ns);
		return *this;
	}

	OpenGLShaderDataSegment& OpenGLShaderDataSegment::Declare(
		ResourceFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns) {
		VALIDATE_STATE_AND_STAGE
		if (HasConflictingFlags(flags, ResourceFlags::Buffer, ResourceFlags::Texture)) {
			throw std::invalid_argument("Invalid or ambiguous ResourceFlags");
		}
		declarations->emplace_back(where, Declaration::Binding{ flags, count }, visible, ns);
		return *this;
	}

	OpenGLShaderDataSegment& OpenGLShaderDataSegment::Declare(
		SamplerFlags flags, SamplerAttachmentInfo const& info, std::size_t where,
		ShaderStage visible, std::size_t ns) {
		VALIDATE_STATE_AND_STAGE
		declarations->emplace_back(where, Declaration::StaticSamplerDeclaration{ flags, info }, visible, ns);
		return *this;
	}

	OpenGLShaderDataSegment& OpenGLShaderDataSegment::Declare(
		SamplerFlags flags, std::size_t where, std::size_t count, ShaderStage visible, std::size_t ns) {
		VALIDATE_STATE_AND_STAGE
		declarations->emplace_back(where, Declaration::Binding{ flags, count }, visible, ns);
		return *this;
	}

#undef VALIDATE_STATE_AND_STAGE

	void OpenGLShaderDataSegment::Instantiate() {
		auto* declarations = std::get_if<std::vector<Declaration>>(&m_handle);
		if (!declarations) {
			throw std::logic_error("Cannot instantiate again after instantiation");
		}

		if (!m_logical_device_id) {
			throw std::runtime_error("Invalid logical device id");
		}
		auto logical_device = plastic::utility::QueryObject<OpenGLLogicalDevice>(*m_logical_device_id);
		if (!logical_device) {
			throw std::runtime_error("Logical device lost");
		}

		std::unordered_map<std::size_t, std::vector<Declaration>> ns_map;
		std::vector<Declaration> push_constants;

		for (auto& decl : *declarations) {
			std::visit(
				[&](auto&& info) {
					using Info = std::decay_t<decltype(info)>;
					if constexpr (std::same_as<Info, Declaration::PushConstant>) {
						push_constants.emplace_back(std::move(decl));
					}
					else if constexpr (std::same_as<Info, Declaration::Binding>) {
						ns_map[decl.ns].emplace_back(std::move(decl));
					}
					else if constexpr (std::same_as<Info, Declaration::StaticSamplerDeclaration>) {
						ns_map[decl.ns].emplace_back(std::move(decl));
					}
				},
				decl.info
			);
		}

		InstantiatedData& instantiated = m_handle.emplace<InstantiatedData>();

		for (auto& decl : push_constants) {
			auto const& pc = std::get<Declaration::PushConstant>(decl.info);
			instantiated.push_constants.emplace_back(decl.where, pc.count * 4, decl.stages);
		}

		MakeCurrent();

		for (auto& [ns, decls] : ns_map) {
			auto [iter, success] = instantiated.set_layouts.try_emplace(ns);
			if (!success) {
				throw std::runtime_error("Failed to set layout");
			}
			InstantiatedData::SetLayout& set_layout = iter->second;
			for (auto& decl : decls) {
				std::visit(
					[&](auto&& info) {
						using Info = std::decay_t<decltype(info)>;
						if constexpr (std::same_as<Info, Declaration::Binding>) {
							set_layout.bindings.emplace_back(
								decl.where,
								info.flags,
								info.count,
								decl.stages
							);
						}
						else if constexpr (std::same_as<Info, Declaration::StaticSamplerDeclaration>) {
							auto [_, success] = set_layout.immutable_samplers.try_emplace(decl.where, *logical_device, info.flags, info.attachment);
							if (!success) {
								throw std::runtime_error("Failed to create sampler");
							}
							set_layout.bindings.emplace_back(
								decl.where,
								info.flags,
								1u,
								decl.stages
							);
						}
						else{

						}
					},
					decl.info
				);
			}
		}

	}

	void OpenGLShaderDataSegment::Reset() {
		m_handle = std::vector<Declaration>();
	}

	std::pair<BindingInfo const*, OpenGLSampler const*> OpenGLShaderDataSegment::GetBindingInfo(
		std::size_t ns, 
		std::size_t where
	) const noexcept {
		auto* instantiated = std::get_if<InstantiatedData>(&m_handle);
		if (!instantiated) {
			return { nullptr, nullptr };
		}
		auto set_it = instantiated->set_layouts.find(ns);
		if (set_it == instantiated->set_layouts.end()) {
			return { nullptr, nullptr };
		}

		auto const& bindings = set_it->second.bindings;
		for (auto const& binding : bindings) {
			if (binding.where == where) {
				return std::visit(
					[&binding, set_it](auto&& flags) -> std::pair<BindingInfo const*, OpenGLSampler const*> {
						using Flags = std::decay_t<decltype(flags)>;
						if constexpr (std::same_as<Flags, SamplerFlags>) {
							for (auto const& [where, sampler] : set_it->second.immutable_samplers) {
								if (binding.where == where) {
									return { &binding, &sampler };
								} 
							}
							return { &binding, nullptr };
						}
						else {
							return { &binding, nullptr };
						}
					},
					binding.flags
				);
			}
		}

		return { nullptr, nullptr };

	}

	std::vector<OpenGLShaderDataSegment::ImmutableSamplerInfo> OpenGLShaderDataSegment::GetImmutableSamplers() const {
		auto* instantiated = std::get_if<InstantiatedData>(&m_handle);
		if (!instantiated) {
			return {};
		}

		std::vector<ImmutableSamplerInfo> result;
		for (auto const& [ns, set_layout] : instantiated->set_layouts) {
			for (auto const& [binding, sampler] : set_layout.immutable_samplers) {
				result.emplace_back(
					static_cast<std::uint32_t>(ns),
					static_cast<std::uint32_t>(binding),
					&sampler
				);
			}
		}
		return result;

	}
	
} // namespace fyuu_rhi::opengl

#endif // !defined(__APPLE__)