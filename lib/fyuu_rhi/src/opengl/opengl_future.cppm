/* opengl_future.cppm */
/**
 * @file opengl_future.cppm
 * @brief OpenGL module partition providing Promise and Future implementations.
 *
 * This module partition defines OpenGL-specific Promise and Future classes,
 * enabling synchronization of GPU commands using OpenGL fence sync objects.
 * OpenGL does not have native timeline semaphores; instead, we use GLsync fences
 * to signal completion of commands on the GPU.
 */
module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <stdexcept>
#include <memory>
#include <atomic>
#include <memory_resource>
#include <concepts>
#endif // !defined(__cpp_lib_modules)
#include "platform.hpp"
#include "glad.hpp"
export module fyuu_rhi:opengl_future;

#if !defined(__APPLE__)

#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
import :types;
import :opengl_common;
import :opengl_logical_device;

namespace {
	/**
	 * @brief Internal interface for type-erased storage of a value set on a promise.
	 *
	 * The Get() method copies the stored value into the provided buffer.
	 */
	struct IValue {
		/**
		 * @brief Copies the stored value into the provided buffer.
		 * @param buffer Pointer to the destination memory. Must be large enough to hold the value.
		 */
		virtual void Get(void* buffer) = 0;
		virtual ~IValue() noexcept = default;
	};
}

namespace fyuu_rhi::opengl {

	export class OpenGLFence 
		: public OpenGLStateMachine {
	private:
		GLsync m_impl;

	public:
		template <std::derived_from<OpenGLStateMachine> Derived>
		OpenGLFence(Derived const* derived)
			: OpenGLStateMachine(derived),
			m_impl(
				[this]() -> GLsync {
					MakeCurrent();
					GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
					if (!sync) {
						throw std::runtime_error("Failed to create fence");
					}
					return sync;
				}()) {
		}

		OpenGLFence(OpenGLFence const&) = delete;
		OpenGLFence& operator=(OpenGLFence const&) = delete;

		OpenGLFence(OpenGLFence&& other) noexcept;

		~OpenGLFence() noexcept;

		GLenum Wait() const noexcept;

	};

	/**
	 * @brief OpenGL-specific promise that can be fulfilled by a GPU operation.
	 *
	 * Manages an OpenGL context (to ensure thread safety) and a type-erased value
	 * that will be delivered to the future. The promise uses GLsync fences to track
	 * completion of command submissions.
	 */
	export class OpenGLPromise
		: public PolymorphicPromiseBase,
		 public OpenGLCommon<OpenGLPromise> {
		friend class OpenGLFuture;
		friend class OpenGLCommandQueueSignal;
	private:
		std::atomic<std::shared_ptr<OpenGLFuture>> m_future; ///< Most recently created future (set by signaling).
		std::unique_ptr<IValue> m_value;					///< Type-erased value set on this promise.

	public:
		/**
		 * @brief Constructs a promise using the given logical device.
		 *
		 * The context is stored to ensure that all OpenGL calls (e.g., fence creation)
		 * are made on the correct thread/context.
		 *
		 * @param logical_device Reference to the OpenGLLogicalDevice that provides the context.
		 */
		explicit OpenGLPromise(OpenGLLogicalDevice const& logical_device);


		OpenGLPromise(OpenGLPromise&& other) noexcept;

		/**
		 * @brief Retrieves and clears the most recent future associated with this promise.
		 *
		 * After this call, the internal future pointer is reset to nullptr.
		 *
		 * @return Shared pointer to the most recent OpenGLFuture, or nullptr if none exists.
		 */
		std::shared_ptr<OpenGLFuture> GetFuture() noexcept;

		/**
		 * @brief Signals the completion of GPU work by creating a fence and associating a new future.
		 *
		 * This function creates a GLsync fence on the correct OpenGL context, then atomically
		 * stores a newly allocated OpenGLFuture that will wait on that fence. The promised value
		 * (if any) is transferred to the future.
		 */
		void Signal() noexcept;

		/**
		 * @brief Sets the value that will be made available through the future.
		 *
		 * The value is type-erased by wrapping it in a derived IValue.
		 *
		 * @tparam Value Type of the value to store.
		 * @param value The value to store (moved into the promise).
		 */
		template <class Value>
		void SetValue(Value&& value) {
			struct ValueImpl final : public IValue {
				Value value;
				void Get(void* buffer) override {
					*static_cast<Value*>(buffer) = std::move(value);
				}
			};
			m_value = std::make_unique<ValueImpl>(std::forward<Value>(value));
		}
	};

	/**
	 * @brief OpenGL-specific future that holds the result of an asynchronous GPU operation.
	 *
	 * Waits on an OpenGL fence sync object and provides access to the promised value.
	 */
	export class OpenGLFuture
		: public PolymorphicFutureBase,
		 public OpenGLCommon<OpenGLFuture> {
	private:
		OpenGLFence m_fence;			   ///< The fence sync object that signals completion.
		std::unique_ptr<IValue> m_value;   ///< Type-erased promised value (moved from the promise).

	public:
		/**
		 * @brief Constructs a future from a promise and a fence that will be signaled when the GPU work completes.
		 *
		 * The fence is moved into the future, and the promised value is transferred from the promise.
		 *
		 * @param promise Pointer to the associated OpenGLPromise.
		 * @param fence   Rvalue reference to an OpenGLFence that will be moved into the future.
		 */
		OpenGLFuture(OpenGLPromise* promise, OpenGLFence&& fence);

		/**
		 * @brief Blocks until the fence has been signaled by the GPU.
		 *
		 * Uses glClientWaitSync with an infinite timeout. Throws on error or timeout.
		 */
		void Wait() const;

		/**
		 * @brief Retrieves the promised value (moves it out of the future) and clears the stored value.
		 *
		 * The caller must specify the expected type. Blocks until the value is ready.
		 *
		 * @tparam Value Expected type of the promised value.
		 * @return The promised value.
		 */
		template <class Value>
		Value Get() {
			Wait(); // Ensure completion before accessing value
			Value buffer;
			m_value->Get(&buffer);
			m_value = nullptr;
			return buffer;
		}
	};
}

#endif // !defined(__APPLE__)