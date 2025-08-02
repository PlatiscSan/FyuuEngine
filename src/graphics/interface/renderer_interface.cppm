export module renderer_interface;
import std;

export namespace graphics {
    class IShader {
    public:
        virtual ~IShader() = default;
        virtual void Bind() const = 0;
        // virtual void SetUniform(const char* name, const Mat4& value) = 0;
    };

    class ITexture {
    public:
        virtual ~ITexture() = 0;
        virtual void Bind(std::uint32_t slot) const = 0;
    };

    class IVertexBuffer {
    public:
        virtual ~IVertexBuffer() = 0;
        virtual void Bind() const = 0;
        virtual void SetData(void const* data, std::size_t size) = 0;
    };

    class IRenderDevice {
    public:
        virtual ~IRenderDevice() = default;

        virtual std::unique_ptr<IShader> CreateShader(std::filesystem::path const& vs_src, std::filesystem::path const& fs_src) = 0;
        virtual std::unique_ptr<ITexture> CreateTexture(int width, std::uint32_t height, void const* data) = 0;
        virtual std::unique_ptr<IVertexBuffer> CreateVertexBuffer() = 0;

        virtual void Clear(float r, float g, float b, float a) = 0;
        virtual void SetViewport(int x, int y, std::uint32_t width, std::uint32_t height) = 0;
        virtual void DrawTriangles(std::uint32_t vertex_count) = 0;
    };
}