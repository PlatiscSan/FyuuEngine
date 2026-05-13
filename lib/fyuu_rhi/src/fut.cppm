module;
#include <version>
#if !defined(__cpp_lib_modules)
#include <exception>
#include <type_traits>
#include <limits>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <concepts>
#include <coroutine>
#include <semaphore>
#endif // !defined(__cpp_lib_modules)
#include <boost/lockfree/queue.hpp>
#include <boost/any.hpp>
#include <boost/any/basic_any.hpp>
export module fyuu_rhi:future;
#if defined(__cpp_lib_modules)
import std;
#endif // defined(__cpp_lib_modules)
namespace fyuu_rhi {

	export using Any = boost::anys::basic_any<32u, 8u>;

	export struct AsyncFlag {};

	export class Dispatchable {
	protected:
		class Resumable {
		public:
			virtual ~Resumable() noexcept = default;
			virtual bool ResumeIfReady() = 0;
		};

		class Scheduler {
		private:
			static constexpr std::size_t Capacity = (std::numeric_limits<std::uint16_t>::max)() - 1u;
			using Queue = boost::lockfree::queue<Resumable*, boost::lockfree::capacity<Capacity>>;
			using Semaphore = std::counting_semaphore<Capacity>;
			std::atomic<Queue*> m_queue;
			std::atomic<Semaphore*> m_semaphore;
			std::jthread m_worker;

			Semaphore* LoadSemaphore() noexcept {
				Semaphore* semaphore;
				do {
					semaphore = m_semaphore.load(std::memory_order::relaxed);
				} while (!semaphore);
				return semaphore;
			}

			Queue* LoadQueue() noexcept {
				Queue* queue;
				do {
					queue = m_queue.load(std::memory_order::relaxed);
				} while (!queue);
				return queue;
			}

		public:
			Scheduler()
				: m_queue(nullptr),
				m_semaphore(nullptr),
				m_worker(
					[this](std::stop_token st) {

						Queue queue;
						Semaphore semaphore{ 0u };

						Resumable* r = nullptr;
						m_queue.store(&queue, std::memory_order::relaxed);
						m_semaphore.store(&semaphore, std::memory_order::relaxed);

						std::size_t consecutive_failures = 0;
						static constexpr std::size_t yield_threshold = 10;
						static constexpr std::size_t sleep_threshold = 30;
						static constexpr std::size_t max_sleep_us = 1000;

						while (!st.stop_requested()) {
							if (queue.pop(r)) {
								if (r->ResumeIfReady()) {
									delete r;
									consecutive_failures = 0;
								}
								else {
									queue.push(r);
									semaphore.release();

									++consecutive_failures;
									if (consecutive_failures > yield_threshold) {
										if (consecutive_failures > sleep_threshold) {
											std::size_t sleep_us = static_cast<std::size_t>(1u) << (consecutive_failures - sleep_threshold);
											if (sleep_us > max_sleep_us) {
												sleep_us = max_sleep_us;
											}
											std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));
										}
										else {
											std::this_thread::yield();
										}
									}
								}
							}
							else {
								consecutive_failures = 0;
								semaphore.acquire();
							}
						}

						while (queue.pop(r)) {
							delete r;
						}

					}
				) {

			}

			~Scheduler() noexcept {
				if (auto semaphore = LoadSemaphore()) {
					m_worker.get_stop_source().request_stop();
					semaphore->release();
				}
			}

			template <class F>
				requires std::is_invocable_r_v<bool, F>
			void Enqueue(std::coroutine_handle<> h, F&& IsReady) {
				class Derived : public Resumable {
				private:
					std::coroutine_handle<> m_coro;
					F IsReady;

				public:
					Derived(std::coroutine_handle<> coro, F& IsReady_)
						: m_coro(coro), IsReady(IsReady_) {

					}

					Derived(std::coroutine_handle<> coro, F&& IsReady_)
						: m_coro(coro), IsReady(std::move(IsReady_)) {

					}

					~Derived() noexcept {
						m_coro.destroy();
					}

					bool ResumeIfReady() override {
						if (IsReady()) {
							m_coro();
							return true;
						}
						return false;
					}

				};

				Semaphore* semaphore = LoadSemaphore(); 
				Queue* queue = LoadQueue();

				queue->push(new Derived(h, std::forward<F>(IsReady)));
				semaphore->release();

			}

		};

		static inline Scheduler* s_scheduler;

		Scheduler* GetScheduler() {
			static std::once_flag scheduler_init;
			static Scheduler scheduler;
			std::call_once(
				scheduler_init,
				[]() {
					s_scheduler = &scheduler;
				}
			);
			return s_scheduler;
		}
		
	};

	export template <class Backend> class Future : public Dispatchable {
	public:
		using Implementation = typename Backend::Future;

	private:
		Implementation m_impl;
		Any m_val;

	public:
		template <std::convertible_to<Implementation> I>
		Future(I&& impl, Any&& val)
			: m_impl(std::forward<I>(impl)),
			m_val(std::move(val)) {

		}

		template <class T>
		T GetValue() {
			T val = std::move(boost::any_cast<T&>(m_val));
			m_val.clear();
			return val;
		}

		void Wait() const {
			if (requires { Backend::WaitFuture(m_impl); }) {
				using Ret = decltype(Backend::WaitFuture(m_impl));
				static_assert(std::same_as<void, Ret>, "Backend::WaitFuture() must return void");
				Backend::WaitFuture(m_impl);
			}
		}

		template <class T>
		auto Wait(AsyncFlag) const {
			
			struct Awaitable {
				Scheduler* scheduler;
				Implementation impl;
				Any val;
				std::exception_ptr ex;

				bool await_ready() const noexcept {
					return Backend::IsFutureReady(impl);
				}

				std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) noexcept {
					try {
						scheduler->Enqueue(h, [this]() { return Backend::IsFutureReady(impl); });
						return std::noop_coroutine();
					}
					catch (std::exception const&) {
						ex = std::current_exception();
						return h;
					}
					
				}

				T await_resume() {
					if (ex) {
						std::rethrow_exception(ex);
					}
					T result = std::move(boost::any_cast<T&>(val));
					val.clear();
					return result;
				}

			};

			return Awaitable{ GetScheduler(), m_impl, std::move(m_val), nullptr };

		}

	};

	export template <class Backend> class Promise {
	public:
		using Implementation = typename Backend::Promise;

	private:
		Implementation m_impl;
		Any m_val;

	public:
		template <std::convertible_to<Implementation> I>
		Promise(I&& impl)
			: m_impl(std::forward<I>(impl)) {

		}

		template <class T>
		void SetValue(T&& val) {
			m_val = std::forward<T>(val);
		}

		Future<Backend> GetFuture() noexcept {
			using Ret = decltype(Backend::GetFuture(m_impl));
			static_assert(std::constructible_from<Future<Backend>, Ret, Any>,
				"Future<Backend> must be constructible from future handle returned by Backend::GetFuture()");
			return { Backend::GetFuture(m_impl), std::move(m_val) };
		}

	};
}