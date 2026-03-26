/* opengl_future.impl.cpp */
/**
 * @file opengl_future.impl.cpp
 * @brief Implementation of OpenGLPromise and OpenGLFuture.
 *
 * This file contains the definitions of the OpenGL promise/future classes,
 * including fence creation, waiting, and allocation strategies.
 */
module;
#include <version>
#include "platform.hpp"
#include "glad.hpp"
#if !defined(__cpp_lib_modules)
#include <memory>
#include <atomic>
#include <memory_resource>
#endif // !defined(__cpp_lib_modules)

module fyuu_rhi:opengl_future_impl;
#if !defined(__APPLE__)
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :opengl_future;
import :opengl_common;

namespace {
	using namespace fyuu_rhi::opengl;

	/**
	 * @brief Memory pool for OpenGLFuture allocations.
	 *
	 * A synchronized pool reduces allocation overhead and avoids contention.
	 */
	std::pmr::synchronized_pool_resource m_future_pool{};

	/**
	 * @brief Polymorphic allocator for OpenGLFuture, using the pool above.
	 */
	std::pmr::polymorphic_allocator<OpenGLFuture> s_future_alloc{ &m_future_pool };
}

namespace fyuu_rhi::opengl {

	OpenGLFence::OpenGLFence(OpenGLFence&& other) noexcept
		: OpenGLStateMachine(std::move(other)),
		m_impl(std::exchange(other.m_impl, nullptr)) {
	}

	fyuu_rhi::opengl::OpenGLFence::~OpenGLFence() noexcept {
		if (m_impl) {
			MakeCurrent();
			glDeleteSync(m_impl);
		}
	}

	GLenum OpenGLFence::Wait() const noexcept {
		MakeCurrent();
		return glClientWaitSync(m_impl, 0u, (std::numeric_limits<GLuint64>::max)());
	}

	/**
	 * @brief Constructs an OpenGLPromise.
	 *
	 * Stores the shared context from the logical device to ensure thread-safe
	 * OpenGL operations.
	 *
	 * @param logical_device The logical device providing the OpenGL context.
	 */
	OpenGLPromise::OpenGLPromise(OpenGLLogicalDevice const& logical_device)
		: PolymorphicPromiseBase(this),
		OpenGLCommon(this, logical_device),
		m_future(nullptr),
		m_value(nullptr) {

	}

	OpenGLPromise::OpenGLPromise(OpenGLPromise&& other) noexcept
		: PolymorphicPromiseBase(std::move(other)),
		OpenGLCommon(std::move(other)),
		m_future(other.m_future.exchange(nullptr, std::memory_order::relaxed)),
		m_value(std::move(other.m_value)) {
	}

	/**
	 * @brief Atomically exchanges the stored future with nullptr and returns it.
	 * @return The most recent future, or nullptr.
	 */
	std::shared_ptr<OpenGLFuture> OpenGLPromise::GetFuture() noexcept {
		// Exchange the stored future with nullptr and return the old value.
		// This ensures that each future can be retrieved only once.
		return m_future.exchange(nullptr, std::memory_order::relaxed);
	}

	/**
	 * @brief Signals the GPU work completion by creating a fence and a new future.
	 *
	 * Creates a GLsync fence on the correct OpenGL context, then allocates
	 * a new OpenGLFuture that will wait on that fence. The future is stored
	 * atomically in the promise.
	 */
	void OpenGLPromise::Signal() noexcept {

		// Create a fence sync object on the correct OpenGL context.
		OpenGLFence fence(this);

		// Atomically store the newly created future in the promise.
		// The future is allocated using the pool allocator and shared ownership.
		m_future.store(
			std::allocate_shared<OpenGLFuture>(s_future_alloc, this, std::move(fence)),
			std::memory_order::relaxed
		);
	}

	/**
	 * @brief Constructs an OpenGLFuture by taking ownership of a fence and the promised value.
	 *
	 * @param promise Pointer to the associated promise.
	 * @param fence   Rvalue reference to an OpenGLFence (moved into the future).
	 */
	OpenGLFuture::OpenGLFuture(OpenGLPromise* promise, OpenGLFence&& fence)
		: PolymorphicFutureBase(this),
		  OpenGLCommon(this, promise),
		  m_fence(std::move(fence)),				  // Take ownership of the fence.
		  m_value(std::move(promise->m_value)) {	  // Transfer the promised value from the promise.
	}

	/**
	 * @brief Blocks until the fence has been signaled by the GPU.
	 *
	 * Calls glClientWaitSync with an infinite timeout. Throws an exception
	 * if the wait fails or times out (should not happen with infinite timeout).
	 */
	void OpenGLFuture::Wait() const {
		MakeCurrent();
		// Wait indefinitely for the fence to become signaled.
		GLenum wait_return = m_fence.Wait();

		// Interpret the result of glClientWaitSync.
		switch (wait_return) {
		case GL_CONDITION_SATISFIED:
		case GL_ALREADY_SIGNALED:
			// Fence is signaled, success.
			break;
		case GL_TIMEOUT_EXPIRED:
			// Should not happen with infinite timeout, but handle defensively.
			throw std::runtime_error("OpenGLFuture::Wait: wait timed out");
		case GL_WAIT_FAILED:
			// An error occurred during wait.
			throw std::runtime_error("OpenGLFuture::Wait: unknown error");
		}
	}
}

#endif // !defined(__APPLE__)