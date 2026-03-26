/* d3d12_swap_chain.impl.cpp */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <utility>
#include <vector>
#include <optional>
#include <algorithm>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include "declare_pool.hpp"

module fyuu_rhi:d3d12_swap_chain_impl;
#if defined(_WIN32)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :d3d12_swap_chain;
import :types;
import :d3d12_types;
import :d3d12_throw;

import :d3d12_physical_device;
import :d3d12_logical_device;
import :d3d12_command_queue;
import :d3d12_surface;
import :d3d12_future;

namespace fyuu_rhi::d3d12 {

	D3D12SwapChain::Frame::Frame(D3D12LogicalDevice const& logical_device, D3D12SwapChain const& swap_chain, std::size_t index)
		: texture(logical_device, swap_chain.GetRawNative(), index),
		view(logical_device, texture, 0u, 1u, 0u, 1u),
		future() {
	}

	void D3D12SwapChain::Resize(std::size_t width, std::size_t height, std::size_t back_buffer_size) {
		// Don't allow 0 size swap chain back buffers.
		width = (std::max)(std::size_t(1), width);
		height = (std::max)(std::size_t(1), height);

#define CHECK_GET_D3D12_OBJECT(id, Type, var, name)\
		if (!id) throw std::invalid_argument("Invalid " name);\
		Type* var = plastic::utility::QueryObject<Type>(*id);\
		if (!var) throw std::runtime_error(name " lost");

		CHECK_GET_D3D12_OBJECT(m_logical_device_id, D3D12LogicalDevice, logical_device, "logical device")
		CHECK_GET_D3D12_OBJECT(m_present_queue_id, D3D12CommandQueue, present_queue, "present queue")

		// Flush the queue to make sure the swap chain's back buffers
		// are not being referenced by an in-flight command list.
		present_queue->WaitUntilCompleted();

		// need to release all existing references to the render targes before can resize
		m_frames.clear();

		DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
		ThrowIfFailed(m_impl->GetDesc(&swap_chain_desc));
		ThrowIfFailed(
			m_impl->ResizeBuffers(
				back_buffer_size, 
				width, height, 
				swap_chain_desc.BufferDesc.Format, 
				swap_chain_desc.Flags
			)
		);

		for (std::size_t i = 0; i < back_buffer_size; ++i) {
			m_frames.emplace_back(*logical_device, *this, i);
		}

    }

	D3D12SwapChain::D3D12SwapChain(
		D3D12PhysicalDevice const& physical_device,
		D3D12LogicalDevice const& logical_device,
		D3D12CommandQueue const& present_queue,
		D3D12Surface const& surface,
		SwapChainOption const& swap_chain_option
	) : PolymorphicSwapChainBase(this),
		D3D12Common(this),
		m_impl(
			[&physical_device, &present_queue, &surface, &swap_chain_option]() {

				IDXGIFactory5* factory = physical_device.GetRawFactory();

				Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain4;
				BOOL allow_tearing;

				HRESULT result = factory->CheckFeatureSupport(
					DXGI_FEATURE_PRESENT_ALLOW_TEARING,
					&allow_tearing,
					sizeof(allow_tearing)
				);

				if (FAILED(result)) {
					allow_tearing = FALSE;
				}

				auto [width, height] = surface.GetSize();
				HWND window_handle = surface.GetPlatformHandle().impl;

				DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
				swap_chain_desc.Width = width;
				swap_chain_desc.Height = height;
				swap_chain_desc.Format = DetermineFormat(swap_chain_option.back_buffer_flags);
				swap_chain_desc.Stereo = false;
				swap_chain_desc.SampleDesc = { 1, 0 };
				swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				swap_chain_desc.BufferCount = swap_chain_option.back_buffer_size;
				swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;               // what hapens when the back buffer is not the size of the target
				swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;   // for going as fast as possible (no vsync), discard frames in-flight to reduce latency
				swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
				// It is recommended to always allow tearing if tearing support is available.
				swap_chain_desc.Flags = allow_tearing == TRUE ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

				Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain1;

				ThrowIfFailed(
					factory->CreateSwapChainForHwnd(
						present_queue.GetRawNative(),
						window_handle,
						&swap_chain_desc,
						nullptr,
						nullptr,
						&swap_chain1
					)
				);

				ThrowIfFailed(swap_chain1.As(&swap_chain4));

				return swap_chain4;

			}()),
		m_internal_promise(logical_device),
		m_logical_device_id(logical_device.GetID()),
		m_present_queue_id(present_queue.GetID()),
		m_surface_id(surface.GetID()),
		m_option(swap_chain_option) {
		auto [width, height] = surface.GetSize();
		Resize(width, height, m_option.back_buffer_size);
	}

	D3D12SwapChain::~D3D12SwapChain() noexcept {

		if (!m_present_queue_id) {
			return;
		}

		D3D12CommandQueue* present_queue = plastic::utility::QueryObject<D3D12CommandQueue>(*m_present_queue_id); 
		
		if (!present_queue) {
			return;
		}

		present_queue->WaitUntilCompleted();

	}

	void D3D12SwapChain::Resize(std::optional<std::size_t> const& back_buffer_size) {

		if (!m_surface_id) {
			throw std::runtime_error("invalid surface");
		}

		D3D12Surface* surface = plastic::utility::QueryObject<D3D12Surface>(*m_surface_id);
		if (!surface) {
			throw std::runtime_error("surface lost");
		}

		auto [width, height] = surface->GetSize();

		if (back_buffer_size){
			Resize(width, height, *back_buffer_size);
			m_option.back_buffer_size = *back_buffer_size;
		}
		else {
			Resize(width, height, m_option.back_buffer_size);
		}

    }

	std::tuple<D3D12Resource*, D3D12View*, std::shared_ptr<D3D12Future>> D3D12SwapChain::GetNextFrame() {
		std::size_t current_index = m_impl->GetCurrentBackBufferIndex();
		auto& frame = m_frames[current_index];
		return { &frame.texture, &frame.view, frame.future } ;
	}

    void D3D12SwapChain::Present(std::shared_ptr<D3D12Future> const& future) {

		std::size_t current_index = m_impl->GetCurrentBackBufferIndex();

		CHECK_GET_D3D12_OBJECT(m_present_queue_id, D3D12CommandQueue, present_queue, "present queue")

		if (future) {
			future->Wait();
		}

		DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
		ThrowIfFailed(m_impl->GetDesc1(&swap_chain_desc));

		bool is_tearing_supported = (swap_chain_desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) != 0u;

		UINT sync_interval = m_option.enable_v_sync ? 1 : 0;
		UINT present_flags = (is_tearing_supported && !m_option.enable_v_sync) ? DXGI_PRESENT_ALLOW_TEARING : 0;
		ThrowIfFailed(m_impl->Present(sync_interval, present_flags));

		m_internal_promise.CommandQueueSignal(present_queue);
		m_frames[current_index].future = m_internal_promise.GetFuture();

    }

    void D3D12SwapChain::SetVSync(bool mode) {
		m_option.enable_v_sync = mode;
    }

    IDXGISwapChain4 *D3D12SwapChain::GetRawNative() const noexcept {
        return m_impl.Get();
    }
}

#endif // defined(_WIN32)