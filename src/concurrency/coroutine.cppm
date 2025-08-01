export module coroutine;
import std;
import defer;

export namespace concurrency {

	template <class T>
	struct GetCoroutinePromise {
		using type = void;
	};

	template <class Promise>
	struct GetCoroutinePromise<std::coroutine_handle<Promise>> {
		using type = Promise;
	};

	template <class T>
	using CoroutinePromiseType = typename GetCoroutinePromise<T>::type;

	template <class CoroutineHandle>
	struct IsTask {

		using Promise = CoroutinePromiseType<CoroutineHandle>;

		static constexpr bool has_add_reference =
			std::is_member_function_pointer_v<decltype(&Promise::AddReference)>;

		static constexpr bool has_release =
			std::is_member_function_pointer_v<decltype(&Promise::Release)>;

		static constexpr bool value =
			has_add_reference && has_release;
	};

	class InvalidResult : public std::runtime_error {
	public:
		InvalidResult()
			: runtime_error("invalid result") {

		}
	};

	template <class T>
	struct BasePromise {
		std::exception_ptr exception;
		std::atomic_size_t ref_count = 1u;
		std::atomic_flag result_ready = {};

		[[noreturn]]
		inline void AddReference() noexcept {
			ref_count.fetch_add(1u, std::memory_order::release);
		}

		inline void Release() noexcept {
			if (ref_count.fetch_sub(1u, std::memory_order::acq_rel) == 1u) {
				std::coroutine_handle<BasePromise<T>>::from_promise(*this).destroy();
			}
		}

		inline void unhandled_exception() {
			exception = std::current_exception();
		}


	};

	template <class T>
	class AsyncTask {
	public:
		template <class U>
		struct Promise : public BasePromise<U> {
			std::optional<U> result;

			inline AsyncTask get_return_object() noexcept {
				return AsyncTask(std::coroutine_handle<Promise>::from_promise(*this));
			}

			static constexpr auto initial_suspend() noexcept {
				return std::suspend_never();
			}

			template <std::convertible_to<U> Value>
			[[noreturn]]
			inline void return_value(Value&& value) noexcept {
				result.emplace(std::forward<Value>(value));
				this->result_ready.test_and_set(std::memory_order::release);
				this->result_ready.notify_all();
			}

			static constexpr auto final_suspend() noexcept {
				return std::suspend_always();
			}

		};

		template <>
		struct Promise<void> : public BasePromise<void> {

			inline AsyncTask get_return_object() noexcept {
				return AsyncTask(std::coroutine_handle<Promise>::from_promise(*this));
			}

			static constexpr auto initial_suspend() noexcept {
				return std::suspend_never();
			}

			[[noreturn]]
			inline void return_void() noexcept {
				this->result_ready.test_and_set(std::memory_order::release);
				this->result_ready.notify_all();
			}

			static constexpr auto final_suspend() noexcept {
				return std::suspend_always();
			}

		};

		using promise_type = Promise<T>;
		using Handle = std::coroutine_handle<promise_type>;

	private:
		Handle m_handle;

		explicit AsyncTask(Handle handle)
			:m_handle(handle) {

		}

	public:
		~AsyncTask() noexcept {
			m_handle.promise().Release();
		}

		constexpr operator bool() const noexcept {
			return m_handle != nullptr;
		}

		[[noreturn]]
		void Wait() const noexcept {

			if (!m_handle) {
				return;
			}

			auto& promise = m_handle.promise();
			bool result_ready;
			do {
				result_ready = promise.result_ready.test(std::memory_order::acquire);
				if (!result_ready) {
					promise.result_ready.wait(false, std::memory_order::relaxed);
				}
			} while (!result_ready);

		}

		template <class = typename std::enable_if_t<!std::is_same_v<T, void>>>
		std::optional<T> Result() const noexcept {

			if (!m_handle) {
				return std::nullopt;
			}

			Wait();

			auto& promise = m_handle.promise();

			if (promise.exception) {
				std::rethrow_exception(promise.exception);
			}

			return promise.result;

		}

		template <class = typename std::enable_if_t<!std::is_same_v<T, void>>>
		operator T() {
			if (auto result = Result()) {
				return *result;
			}
			throw InvalidResult();
		}

	};

	template <class T>
	struct BaseContinuationPromise : public BasePromise<T> {

		struct FinalAwaiter {

			static constexpr bool await_ready() noexcept {
				return false;
			}

			template <class DerivedPromise>
			inline std::coroutine_handle<> await_suspend(std::coroutine_handle<DerivedPromise> finished) const noexcept {
				auto& promise = finished.promise();
				auto& continuation = promise.continuation;
				return continuation ? continuation : std::noop_coroutine();
			}

			[[noreturn]]
			static constexpr void await_resume() noexcept {

			}

		};

		
		std::coroutine_handle<> continuation;
		std::atomic_flag executed = {};

		static constexpr auto initial_suspend() noexcept {
			return std::suspend_always();
		}

		inline auto final_suspend() noexcept {
			return FinalAwaiter();
		}

	};

	template <class T>
	class Task {
	public:
		template <class U>
		struct Promise : public BaseContinuationPromise<U> {
			std::optional<U> result;

			inline Task get_return_object() noexcept {
				return Task(std::coroutine_handle<Promise>::from_promise(*this));
			}

			template <std::convertible_to<U> Value>
			inline void return_value(Value&& value) noexcept {
				result.emplace(std::forward<Value>(value));
				this->result_ready.test_and_set(std::memory_order::release);
				this->result_ready.notify_all();
			}

		};

		template <>
		struct Promise<void> : public BaseContinuationPromise<void> {

			inline Task get_return_object() noexcept {
				return Task(std::coroutine_handle<Promise>::from_promise(*this));
			}

			[[noreturn]]
			inline void return_void() noexcept {
				this->result_ready.test_and_set(std::memory_order::release);
				this->result_ready.notify_all();
			}

		};

		using promise_type = Promise<T>;
		using Handle = std::coroutine_handle<promise_type>;

		struct TaskAwaiter {

			Handle handle;
			std::function<void()> Release;

			bool await_ready() const noexcept {
				return handle.done();
			}

			template <class CoroutineHandle>
			std::coroutine_handle<> await_suspend(CoroutineHandle continuation) noexcept {
				
				if (handle.done()) {
					return continuation;
				}

				auto& promise = handle.promise();
				promise.continuation = continuation;
				
				if constexpr (IsTask<CoroutineHandle>::value) {
					continuation.promise().AddReference();
					Release = [continuation]() {
						continuation.promise().Release();
						};
				}

				return handle;

			}

			T await_resume() const noexcept {

				util::Defer gc = [this]() {
					if (Release) {
						Release();
						handle.promise().Release();
					}
					};

				auto& promise = handle.promise();

				if (promise.exception) {
					std::rethrow_exception(promise.exception);
				}

				if constexpr (!std::is_same_v<T, void>) {
					return std::move(*(promise.result));
				}

			}


		};

	private:
		Handle m_handle;

		explicit Task(Handle handle)
			:m_handle(handle) {
			m_handle.promise().AddReference();
		}

	public:
		Task(Task const& other) = delete;
		Task& operator=(Task const& other) = delete;

		Task(Task&& other)
			:m_handle(std::move(other.m_handle)) {
		}

		~Task() noexcept {
			if (m_handle) {
				m_handle.promise().Release();
			}
		}

		constexpr operator bool() const noexcept {
			return m_handle != nullptr;
		}	

		Task& operator=(Task&& other) {

			if (this != &other) {
				if (m_handle) {
					m_handle.promise().Release();
				}
				m_handle = std::exchange(other.m_handle, nullptr);
			}

			return *this;

		}

		[[noreturn]]
		void Wait() const noexcept {

			if (!m_handle) {
				return;
			}

			auto& promise = m_handle.promise();
			bool result_ready;
			do {
				result_ready = promise.result_ready.test(std::memory_order::acquire);
				if (!result_ready) {
					promise.result_ready.wait(false, std::memory_order::relaxed);
				}
			} while (!result_ready);

		}

		void operator()() const {

			if (!m_handle || m_handle.done()) {
				return;
			}

			if (m_handle.promise().executed.test_and_set(std::memory_order::acq_rel)) {
				m_handle();
			}

		}

		template <class = typename std::enable_if_t<!std::is_same_v<T, void>>>
		std::optional<T> Result() const noexcept {

			if (!m_handle) {
				return std::nullopt;
			}

			Wait();

			auto& promise = m_handle.promise();

			if (promise.exception) {
				std::rethrow_exception(promise.exception);
			}

			return promise.result;

		}

		template <class = typename std::enable_if_t<!std::is_same_v<T, void>>>
		operator T() {
			(*this)();
			if (auto result = Result()) {
				return *result;
			}
			throw InvalidResult();
		}

		friend decltype(auto) operator co_await(Task&& task) noexcept {
			return TaskAwaiter{ .handle = task.m_handle };
		}

	};

} // namespace common_development::concurrency

