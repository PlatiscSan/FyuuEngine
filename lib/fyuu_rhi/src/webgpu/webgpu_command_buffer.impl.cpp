/* webgpu_command_buffer.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <memory>
#include <vector>
#include <string_view>
#include <span>
#endif // !defined(__cpp_lib_modules)
#include "webgpu.hpp"
module fyuu_rhi:webgpu_command_buffer_impl;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :webgpu_command_buffer;
import :types;
import :webgpu_common;
import :webgpu_future;
import :webgpu_gpu_executable;
import :webgpu_resource;
import :webgpu_view;
import :webgpu_sampler;

namespace fyuu_rhi::webgpu {
	WebGPUCommandBuffer::WebGPUCommandBuffer(WebGPULogicalDevice const &logical_device, CommandObjectType queue_type, std::string_view debug_name)
		: PolymorphicCommandBufferBase(this), 
		WebGPUCommon(this),
		m_commands(),
		m_sync(),
		m_recording_flag() {
	}

	
	WebGPUCommandBuffer& WebGPUCommandBuffer::BeginRecording() {

		m_sync.Acquire();

		if (!std::exchange(m_recording_flag, true)) {
			m_commands.clear();
		}

		return *this;	
		
	}

	void WebGPUCommandBuffer::EndRecording() {

		m_sync.ThrowIfNotOwned();
		std::exchange(m_recording_flag, false);
		m_sync.Release();

	}

	void WebGPUCommandBuffer::Reset() {
		m_sync.ThrowIfNotOwned();
		m_commands.clear();
	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::SetFrame(WebGPUResource const& frame, WebGPUView const& frame_view) {
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdSetFrame>();
		command->texture_id = frame.GetID();
		command->view_id = frame_view.GetID();

		m_commands.emplace_back(std::move(command));

		return *this;
	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::BeginRendering(std::span<WebGPUResource const* const> rts, std::span<WebGPUView const* const> rtvs, std::span<LoadOp const> rt_loads, std::span<StoreOp const> rt_stores, std::span<std::array<float, 4u> const> rt_clears) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdBeginRendering>();

		std::size_t count = (std::min)({ rts.size(), rtvs.size(), rt_loads.size(), rt_stores.size(), rt_clears.size() });

		for (std::size_t i = 0; i < count; ++i) {
			command->render_targets.emplace_back(
				rts[i]->GetID(),	// texture_id
				rtvs[i]->GetID(),	// view_id
				rt_clears[i],		// rt_clear
				rt_loads[i],		// rt_load
				rt_stores[i]		// rt_store
			);
		}
	
		m_commands.emplace_back(std::move(command));
		return *this;

	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::BeginRendering(std::span<WebGPUResource const* const> rts, std::span<WebGPUView const* const> rtvs, std::span<LoadOp const> rt_loads, std::span<StoreOp const> rt_stores, std::span<std::array<float, 4u> const> rt_clears, WebGPUResource const& ds, WebGPUView const& dsv, LoadOp depth_load, StoreOp depth_store, float depth_clear, LoadOp stencil_load, StoreOp stencil_store, std::uint8_t stencil_clear) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdBeginRendering>();

		std::size_t count = (std::min)({ rts.size(), rt_loads.size(), rt_stores.size(), rt_clears.size() });

		for (std::size_t i = 0; i < count; ++i) {
			command->render_targets.emplace_back(
				rts[i]->GetID(),	// texture_id
				rtvs[i]->GetID(),	// view_id
				rt_clears[i],		// rt_clear
				rt_loads[i],		// rt_load
				rt_stores[i]		// rt_store
			);
		}
		command->depth_stencil.emplace(
			ds.GetID(),				// texture_id
			dsv.GetID(),			// view_id
			depth_clear,			// depth_clear
			stencil_clear,			// stencil_clear
			depth_load,				// d_load
			depth_store,			// d_store
			stencil_load,			// s_load
			stencil_store			// s_store
		);
	
		m_commands.emplace_back(std::move(command));

		return *this;

	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::EndRendering() {
		m_sync.ThrowIfNotOwned();
		m_commands.emplace_back(plastic::utility::MakeUnique<CmdEndRendering>());
		return *this;
	}

	
	WebGPUCommandBuffer& WebGPUCommandBuffer::SetViewports(std::span<CmdSetViewports::Viewport const> viewports) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdSetViewports>();

		command->viewports.assign(viewports.begin(), viewports.end());

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::SetScissors(std::span<CmdSetScissors::Scissor const> scissors) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdSetScissors>();

		command->scissors.assign(scissors.begin(), scissors.end());

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::Execute(WebGPUGPUExecutable const& gpu_exec) {

		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdExecute>();

		command->gpu_exec_id = gpu_exec.GetID();

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::DispatchComputingTask(std::size_t threads_x, std::size_t threads_y, std::size_t threads_z, std::size_t threads_per_thread_group_x, std::size_t threads_per_thread_group_y, std::size_t threads_per_thread_group_z) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdDispatchComputingTask>();

		command->threads_x = threads_x;
		command->threads_y = threads_y;
		command->threads_z = threads_z;

		command->threads_per_thread_group_x = threads_per_thread_group_x;
		command->threads_per_thread_group_y = threads_per_thread_group_y;
		command->threads_per_thread_group_z = threads_per_thread_group_z;

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	
	WebGPUCommandBuffer& WebGPUCommandBuffer::BindVertexBuffers(std::span<WebGPUResource const* const> vertex_buffers, std::span<std::size_t const> wheres, std::span<std::size_t const> offsets, std::span<std::size_t const> strides) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdBindVertexBuffers>();
		
		std::size_t binding_count = (std::min)({ vertex_buffers.size(), wheres.size(), offsets.size(), strides.size() });
		for (std::size_t i = 0; i < binding_count; ++i) {
			command->bindings.emplace_back(vertex_buffers[i]->GetID(), wheres[i], offsets[i], strides[i]);
		}

		return *this;

	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::DrawInstanced(std::size_t vertices, std::size_t instances, std::size_t start_vertex, std::size_t first_instance) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdDrawInstanced>();
	
		command->vertices = vertices;
		command->instances = instances;
		command->start_vertex = start_vertex;
		command->first_instance = first_instance;

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::DrawIndexed(std::size_t indices, std::size_t instances, std::size_t first_index, std::size_t start_vertex, std::size_t first_instance) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdDrawIndexedInstanced>();
	
		command->indices = indices;
		command->instances = instances;
		command->first_index = first_index;
		command->start_vertex = start_vertex;
		command->first_instance = first_instance;

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::BindBuffer(
		ShaderStage stages,
		std::size_t where,
		WebGPUResource const& buffer,
		std::size_t offset,
		std::size_t ns
	) {

		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdBindBuffer>();
		
		command->id = buffer.GetID();
		command->offset = offset;
		command->where = where;
		command->ns = ns;
		command->stages = stages;

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	WebGPUCommandBuffer &WebGPUCommandBuffer::BindBuffer(ShaderStage stages, std::size_t where, WebGPUResource const& buffer, WebGPUView const& texel_view, std::size_t offset, std::size_t ns) {
		
		m_sync.ThrowIfNotOwned();

		auto command = plastic::utility::MakeUnique<CmdBindBuffer>();
		
		command->id = buffer.GetID();
		command->texel_view_id = texel_view.GetID();
		command->offset = offset;
		command->where = where;
		command->ns = ns;
		command->stages = stages;

		m_commands.emplace_back(std::move(command));

		return *this;

	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::PushData(ShaderStage stages, std::size_t where, std::span<std::byte const> bytes, std::size_t offset, std::size_t ns) {

		m_sync.ThrowIfNotOwned();
		auto command = plastic::utility::MakeUnique<CmdPushData>();

		command->where = where;
		command->ns = ns;
		command->offset = offset;
		command->bytes.assign(bytes.begin(), bytes.end());
		command->stages = stages;

		return *this;

	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::BindSampler(ShaderStage stages, std::size_t where, WebGPUSampler const& sampler, std::size_t ns) {
		
		m_sync.ThrowIfNotOwned();
		auto command = plastic::utility::MakeUnique<CmdBindSampler>();

		command->id = sampler.GetID();
		command->where = where;
		command->ns = ns;
		command->stages = stages;

		return *this;

	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::BindTexture(ShaderStage stages, std::size_t where, WebGPUResource const& texture, WebGPUView const& view, std::size_t ns) {
		
		m_sync.ThrowIfNotOwned();
		auto command = plastic::utility::MakeUnique<CmdBindTexture>();

		command->texture_id = texture.GetID();
		command->view_id = view.GetID();
		command->where = where;
		command->ns = ns;
		command->stages = stages;

		return *this;

	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::CopyTextureToBuffer(
		WebGPUResource const& src_texture,
		WebGPUResource const& dest_buffer,
		CmdCopyTextureToBuffer::Subresource const& src_subresource,
		CmdCopyTextureToBuffer::Offset3D const& src_offset,
		std::size_t dest_offset,
		CmdCopyTextureToBuffer::Extent3D const& extent
	) {
		m_sync.ThrowIfNotOwned();
	
		auto command = plastic::utility::MakeUnique<CmdCopyTextureToBuffer>();
		command->src_id = src_texture.GetID();
		command->dest_id = dest_buffer.GetID();
		command->src_subresource = src_subresource;
		command->src_offset = src_offset;
		command->dest_offset = dest_offset;
		command->extent = extent;
	
		m_commands.emplace_back(std::move(command));
		return *this;
	}
	
	WebGPUCommandBuffer& WebGPUCommandBuffer::CopyBufferToTexture(
		WebGPUResource const& src_buffer,
		WebGPUResource const& dest_texture,
		std::size_t src_offset,
		CmdCopyBufferToTexture::Subresource const& dest_subresource,
		CmdCopyBufferToTexture::Offset3D const& dest_offset,
		CmdCopyBufferToTexture::Extent3D const& extent
	) {
		m_sync.ThrowIfNotOwned();
	
		auto command = plastic::utility::MakeUnique<CmdCopyBufferToTexture>();
		command->src_id = src_buffer.GetID();
		command->dest_id = dest_texture.GetID();
		command->src_offset = src_offset;
		command->dest_subresource = dest_subresource;
		command->dest_offset = dest_offset;
		command->extent = extent;
	
		m_commands.emplace_back(std::move(command));
		return *this;
	}
	
	WebGPUCommandBuffer& WebGPUCommandBuffer::CopyBufferToBuffer(
		WebGPUResource const& src_buffer,
		WebGPUResource const& dest_buffer,
		std::size_t src_offset,
		std::size_t dest_offset,
		std::size_t size
	) {
		m_sync.ThrowIfNotOwned();
	
		auto command = plastic::utility::MakeUnique<CmdCopyBufferToBuffer>();
		command->src_id = src_buffer.GetID();
		command->dest_id = dest_buffer.GetID();
		command->src_offset = src_offset;
		command->dest_offset = dest_offset;
		command->size = size;
	
		m_commands.emplace_back(std::move(command));
		return *this;
	}
	
	WebGPUCommandBuffer& WebGPUCommandBuffer::CopyTextureToTexture(
		WebGPUResource const& src_texture,
		WebGPUResource const& dest_texture,
		CmdCopyTextureToTexture::Subresource const& src_subresource,
		CmdCopyTextureToTexture::Subresource const& dest_subresource,
		CmdCopyTextureToTexture::Offset3D const& src_offset,
		CmdCopyTextureToTexture::Offset3D const& dest_offset,
		CmdCopyTextureToTexture::Extent3D const& extent
	) {
		m_sync.ThrowIfNotOwned();
	
		auto command = plastic::utility::MakeUnique<CmdCopyTextureToTexture>();
		command->src_id = src_texture.GetID();
		command->dest_id = dest_texture.GetID();
		command->src_subresource = src_subresource;
		command->dest_subresource = dest_subresource;
		command->src_offset = src_offset;
		command->dest_offset = dest_offset;
		command->extent = extent;
	
		m_commands.emplace_back(std::move(command));
		return *this;
	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::BindIndexBuffer(
		WebGPUResource const& buffer
	) {
		m_sync.ThrowIfNotOwned();
		auto cmd = plastic::utility::MakeUnique<CmdBindIndexBuffer>();
		cmd->id = buffer.GetID();
		m_commands.emplace_back(std::move(cmd));
		return *this;
	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::ExecuteIndirect(
		WebGPUResource const& arg_buffer,
		std::size_t offset,
		std::size_t draw_count
	) {
		m_sync.ThrowIfNotOwned();
		auto cmd = plastic::utility::MakeUnique<CmdExecuteIndirect>();
		cmd->id = arg_buffer.GetID();
		cmd->offset_into_buffer = offset;
		cmd->draws = draw_count;
		m_commands.emplace_back(std::move(cmd));
		return *this;
	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::ExecuteIndirectIndexed(
		WebGPUResource const& arg_buffer,
		std::size_t offset,
		std::size_t draw_count
	) {
		m_sync.ThrowIfNotOwned();
		auto cmd = plastic::utility::MakeUnique<CmdExecuteIndirectIndexed>();
		cmd->id = arg_buffer.GetID();
		cmd->offset_into_buffer = offset;
		cmd->draws = draw_count;
		m_commands.emplace_back(std::move(cmd));
		return *this;
	}

	WebGPUCommandBuffer& WebGPUCommandBuffer::DispatchIndirect(
		WebGPUResource const& arg_buffer,
		std::size_t offset,
		std::size_t block_x, std::size_t block_y, std::size_t block_z
	) {
		m_sync.ThrowIfNotOwned();
		auto cmd = plastic::utility::MakeUnique<CmdDispatchIndirect>();
		cmd->id = arg_buffer.GetID();
		cmd->offset_into_buffer = offset;
		cmd->block_size_x = block_x;
		cmd->block_size_y = block_y;
		cmd->block_size_z = block_z;
		m_commands.emplace_back(std::move(cmd));
		return *this;
	}

} // namespace fyuu_rhi::webgpu