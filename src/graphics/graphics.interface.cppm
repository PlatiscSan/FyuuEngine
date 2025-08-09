export module graphics:interface;
import std;
export import logger;
export import window;

namespace graphics {

    export enum class API : std::uint8_t {
        DirectX12,
        Vulkan,
        OpenGL,
        Meta,
        Unknown,
    };

    export class BaseRenderDevice {
    protected:
        core::LoggerPtr m_logger;
        platform::IWindow* m_window;

    public:
        template <std::convertible_to<core::LoggerPtr> Logger>
        BaseRenderDevice(Logger&& logger, platform::IWindow& window)
            : m_logger(std::forward<Logger>(logger)),
            m_window(&window) {

        }

        virtual ~BaseRenderDevice() = default;

        virtual void Clear(float r, float g, float b, float a) = 0;
        virtual void SetViewport(int x, int y, std::uint32_t width, std::uint32_t height) = 0;

        virtual bool BeginFrame() = 0;
        virtual void EndFrame() = 0;

        virtual API GetAPI() const noexcept = 0;

    };

}