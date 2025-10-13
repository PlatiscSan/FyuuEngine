export module rendering:renderer;
export import :command_object;
export import disable_copy;
export import message_bus;

namespace fyuu_engine::core {

	export enum class DataType : std::uint8_t {
		Unknown,

		Uint8,
		Uint8_2,
		Uint8_3,
		Uint8_4,

		Int8,
		Int8_2,
		Int8_3,
		Int8_4,

		Uint16,
		Uint16_2,
		Uint16_3,
		Uint16_4,

		Int16,
		Int16_2,
		Int16_3,
		Int16_4,

		Uint32,
		Uint32_2,
		Uint32_3,
		Uint32_4,

		Int32,
		Int32_2,
		Int32_3,
		Int32_4,

		Float16,
		Float16_2,
		Float16_3,
		Float16_4,

		Float32,
		Float32_2,
		Float32_3,
		Float32_4,

		Count,
	};

	export enum class BufferType : std::uint8_t {
		Unknown,
		/// @brief vertex buffer, GPU read only
		Vertex,
		/// @brief index buffer, GPU read only
		Index,
		/// @brief constant buffer, GPU read only
		Constant,
		/// @brief uniform buffer, GPU read only
		Uniform = Constant,


		/// @brief upload CPU to GPU
		Upload,
		/// @brief For read-back from GPU to CPU
		Staging,
		Count
	};

	export struct BufferDescriptor {
		std::size_t size;
		std::size_t alignment;
		BufferType buffer_type;
		DataType data_type;
	};

	export template <class CommandObject, class CollectiveBuffer, class OutputTarget>
		struct RendererTraits {
		using CommandObjectType = CommandObject;
		using CollectiveBufferType = CollectiveBuffer;
		using OutputTargetType = OutputTarget;
	};

	export enum class LogLevel : std::uint8_t {
		Trace,
		Debug,
		Info,
		Warning,
		Error,
		Fatal
	};

	export class IRendererLogger {
	public:
		virtual ~IRendererLogger() noexcept = default;
		virtual void Log(
			LogLevel level, 
			std::string_view message, 
			std::source_location const& loc = std::source_location::current()
		) = 0;
	};

	export using LoggerPtr = util::PointerWrapper<IRendererLogger>;

	export template <class DerivedRenderer, class Traits> class BaseRenderer
		: util::DisableCopy<BaseRenderer<DerivedRenderer, Traits>> {
	protected:
		LoggerPtr m_logger;
		concurrency::SyncMessageBus m_message_bus;

	public:
		BaseRenderer() = default;

		explicit BaseRenderer(LoggerPtr const& logger)
			: m_logger(logger),
			m_message_bus() {

		}

		BaseRenderer(BaseRenderer&& other) noexcept
			: m_logger(std::move(other.m_logger)),
			m_message_bus(std::move(other.m_message_bus)) {
		}

		BaseRenderer& operator=(BaseRenderer&& other) noexcept {
			if (this != &other) {
				m_logger = std::move(other.m_logger);
				m_message_bus = std::move(other.m_message_bus);
			}
			return *this;
		}

		bool BeginFrame() {
			return static_cast<DerivedRenderer*>(this)->BeginFrameImpl();
		}

		void EndFrame() {
			static_cast<DerivedRenderer*>(this)->EndFrameImpl();
		}

		typename Traits::CommandObjectType& GetCommandObject() {
			return static_cast<DerivedRenderer*>(this)->GetCommandObjectImpl();
		}

		typename Traits::CollectiveBufferType CreateBuffer(BufferDescriptor const& desc) {
			return static_cast<DerivedRenderer*>(this)->CreateBufferImpl(desc);
		}

		typename Traits::OutputTargetType OutputTargetOfCurrentFrame() {
			return static_cast<DerivedRenderer*>(this)->OutputTargetOfCurrentFrameImpl();
		}

	};

	export template <class DerivedRendererBuilder, class DerivedRenderer> class BaseRendererBuilder
		: util::DisableCopy<BaseRendererBuilder<DerivedRendererBuilder, DerivedRenderer>> {
	protected:
		LoggerPtr m_logger;

	public:

		std::shared_ptr<DerivedRenderer> Build() {
			return static_cast<DerivedRendererBuilder*>(this)->BuildImpl();
		}

		void Build(std::optional<DerivedRenderer>& buffer) {
			static_cast<DerivedRendererBuilder*>(this)->BuildImpl(buffer);
		}

		/// @brief 
		/// @param buffer 
		void Build(std::span<std::byte> buffer) {
			static_cast<DerivedRendererBuilder*>(this)->BuildImpl(buffer);
		}

		DerivedRendererBuilder& SetLogger(LoggerPtr const& logger) noexcept {
			m_logger = util::MakeReferred(logger);
			return static_cast<DerivedRendererBuilder&>(*this);
		}

	};

}