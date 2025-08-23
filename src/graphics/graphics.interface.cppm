module;

export module graphics:interface;
import std;
export import logger;
export import window;
export import message_bus;

namespace graphics {

	export enum class API : std::uint8_t {
		DirectX12,
		Vulkan,
		OpenGL,
		Meta,
		Unknown,
	};

	enum class ShaderType : std::uint8_t {
		VERTEX,
		PIXEL,
		GEOMETRY,
		HULL,      // Tessellation Control
		DOMAIN,    // Tessellation Evaluation
		COMPUTE
	};

	enum class ShaderFormat : std::uint8_t {
		UNKNOWN,
		DXBC,      // DirectX Bytecode (D3D12)
		DXIL,      // DirectX Intermediate Language (D3D12)
		SPIRV,     // SPIR-V (Vulkan/OpenGL)
		GLSL,      // OpenGL Shading Language
		SOURCE     // From source
	};

	export class ICommandObject {
	public:
		virtual ~ICommandObject() noexcept = default;
		virtual void* GetNativeHandle() const noexcept = 0;
		virtual API GetAPI() const noexcept = 0;
		virtual void StartRecording() = 0;
		virtual void EndRecording() = 0;
		virtual void Reset() = 0;
	};

	export class BaseRenderDevice {
	protected:
		core::LoggerPtr m_logger;
		platform::IWindow* m_window;		
		util::MessageBus m_message_bus;

	public:
		template <std::convertible_to<core::LoggerPtr> Logger>
		BaseRenderDevice(Logger&& logger, platform::IWindow& window)
			: m_logger(std::forward<Logger>(logger)),
			m_window(&window),
			m_message_bus() {

		}

		BaseRenderDevice(BaseRenderDevice const&) = delete;
		BaseRenderDevice(BaseRenderDevice&& other) noexcept
			: m_logger(std::move(other.m_logger)),
			m_window(std::exchange(other.m_window, nullptr)),
			m_message_bus(std::move(other.m_message_bus)) {
		}

		BaseRenderDevice& operator=(BaseRenderDevice const&) = delete;
		BaseRenderDevice& operator=(BaseRenderDevice&& other) noexcept {
			if (this != &other) {
				m_logger = std::move(other.m_logger);
				m_window = std::exchange(other.m_window, nullptr);
				m_message_bus = std::move(other.m_message_bus);
			}
			return *this;
		}

		virtual ~BaseRenderDevice() = default;

		virtual void Clear(float r, float g, float b, float a) = 0;
		virtual void SetViewport(int x, int y, std::uint32_t width, std::uint32_t height) = 0;

		virtual bool BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual API GetAPI() const noexcept = 0;

		virtual ICommandObject& AcquireCommandObject() = 0;

	};

	export class IShader {
	public:
		virtual ~IShader() noexcept = default;

		virtual ShaderType GetType() const noexcept = 0;

		virtual ShaderFormat GetFormat() const noexcept = 0;

		virtual std::string_view GetSource() const noexcept = 0;

		virtual std::string_view GetEntryPoint() const noexcept = 0;

		virtual void Bind(BaseRenderDevice& renderer) const noexcept = 0;

		virtual void Unbind() const noexcept = 0;

		virtual void* GetNativeHandle() const noexcept = 0;

	};

}