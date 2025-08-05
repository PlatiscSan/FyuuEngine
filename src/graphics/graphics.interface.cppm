export module graphics:interface;
import std;

namespace graphics {

    export enum class API : std::uint8_t {
        DirectX12,
        Vulkan,
        OpenGL,
        Meta,
        Unknown,
    };

    export class IRenderDevice {
    public:
        virtual ~IRenderDevice() = default;

        virtual void Clear(float r, float g, float b, float a) = 0;
        virtual void SetViewport(int x, int y, std::uint32_t width, std::uint32_t height) = 0;

        virtual bool BeginFrame() = 0;
        virtual void EndFrame() = 0;

        virtual API GetAPI() const noexcept = 0;

    };

}