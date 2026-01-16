export module plastic.synchronized_function;
import std;
import plastic.ticket_mutex;

namespace plastic::concurrency {

	export template <class> class SynchronizedFunction;

	export template <class Ret, class... Args> class SynchronizedFunction<Ret(Args...)> {
	private:
		struct ICallable {
			virtual ~ICallable() noexcept = default;
			virtual Ret Call(Args... args) = 0;
			virtual std::unique_ptr<ICallable> Clone() const = 0;
			virtual void Clone(ICallable* callable) const = 0;
		};

		template <class F>
		struct CallableImpl : public ICallable {
			F functor;

			explicit CallableImpl(F const& f)
				: functor(f) {

			}

			explicit CallableImpl(F&& f)
				: functor(std::move(f)) {

			}

			Ret Call(Args... args) override {
				return functor(std::forward<Args>(args)...);
			}

			std::unique_ptr<ICallable> Clone() const override {
				return std::make_unique<CallableImpl<F>>(functor);
			}

			void Clone(ICallable* callable) const override {
				new (callable) CallableImpl<F>(functor);
			}

		};

		static constexpr std::size_t StackAllowSize = 16ull;
		using StackAlloc = std::array<std::byte, StackAllowSize>;

		using CallableStorage = std::variant<std::monostate, StackAlloc, std::unique_ptr<ICallable>>;

		CallableStorage m_callable_storage;
		TicketMutex m_mutex;

		template <class F>
		static inline CallableStorage CreateCallable(F&& f) {

			using DecayedF = std::decay_t<F>;

			CallableStorage callable_storage;

			if constexpr (alignof(CallableImpl<DecayedF>) <= alignof(StackAlloc)) {
				StackAlloc& stack_alloc = callable_storage.emplace<StackAlloc>();
				new (&stack_alloc) CallableImpl<DecayedF>(std::forward<F>(f));
			} else {
				callable_storage.emplace<std::unique_ptr<ICallable>>(std::make_unique<CallableImpl<DecayedF>>(std::forward<F>(f)));
			}

			return callable_storage;

		}

		static inline CallableStorage Clone(SynchronizedFunction const& other) {

			std::lock_guard<TicketMutex> lock(other.m_mutex);

			return std::visit(
				[](auto&& callable) -> CallableStorage {

					using T = std::decay_t<decltype(callable)>;

					if constexpr (std::is_same_v<T, StackAlloc>) {

						CallableStorage new_storage;

						ICallable* source_callable = reinterpret_cast<ICallable*>(&callable);
						ICallable* dest_callable = reinterpret_cast<ICallable*>(&new_storage.emplace<StackAlloc>());

						source_callable->Clone(dest_callable);
						return new_storage;

					}
					else if constexpr (std::is_same_v<T, std::unique_ptr<ICallable>>) {
						return CallableStorage(std::in_place_type<std::unique_ptr<ICallable>>, callable->Clone());
					}
					else {
						return CallableStorage(std::in_place_type<std::monostate>);
					}

				},
				other.m_callable_storage
			);

		}

		
	public:
		SynchronizedFunction()
			: m_callable_storage(std::in_place_type<std::monostate>),
			m_mutex() {

		}

		template <class F>
		SynchronizedFunction(F&& f)
			: m_callable_storage(SynchronizedFunction::CreateCallable(std::forward<F>(f))),
			m_mutex() {

		}

		SynchronizedFunction(SynchronizedFunction const& other)
			: m_callable_storage(SynchronizedFunction::Clone(other)),
			m_mutex() {

		}

		SynchronizedFunction(SynchronizedFunction&& other) noexcept
			: m_callable_storage(std::move(other.m_callable_storage)),
			m_mutex() {

		}

		void Reset() noexcept {

			std::lock_guard<TicketMutex> lock(m_mutex);

			std::visit(
				[](auto&& callable) {
					using T = std::decay_t<decltype(callable)>;
					if constexpr (std::is_same_v<T, StackAlloc>) {
						ICallable* ptr = reinterpret_cast<ICallable*>(&callable);
						ptr->~ICallable();
					}
				},
				m_callable_storage
			);

			m_callable_storage.emplace<std::monostate>();

		}

		~SynchronizedFunction() noexcept {
			Reset();
		}

		SynchronizedFunction& operator=(SynchronizedFunction const& other) {

			if (this != &other) {
				Reset();
				m_callable_storage = SynchronizedFunction::Clone(other);
			}

			return *this;

		}

		SynchronizedFunction& operator=(SynchronizedFunction&& other) noexcept {

			if (this != &other) {
				Reset();
				std::lock_guard lock(other.m_mutex);
				m_callable_storage = std::move(other.m_callable_storage);
			}

			return *this;

		}

		Ret operator()(Args... args) {

			std::lock_guard lock(m_mutex);

			if (std::holds_alternative<std::monostate>(m_callable_storage)) {
				throw std::bad_function_call();
			}

			ICallable* callable_ptr = nullptr;

			if (std::holds_alternative<StackAlloc>(m_callable_storage)) {
				StackAlloc& stack_alloc = std::get<StackAlloc>(m_callable_storage);
				callable_ptr = reinterpret_cast<ICallable*>(&stack_alloc);
			} else {
				callable_ptr = std::get<std::unique_ptr<ICallable>>(m_callable_storage).get();
			}

			return callable_ptr->Call(std::forward<Args>(args)...);

		}


	};
	
} // namespace plastic::concurrency
