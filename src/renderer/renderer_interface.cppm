export module renderer_interface;
import std;

export namespace renderer {
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
        virtual void SetData(const void* data, size_t size) = 0;
    };

    class IRenderDevice {
    public:
        virtual ~IRenderDevice() = default;

        virtual std::unique_ptr<IShader> CreateShader(const char* vsSrc, const char* fsSrc) = 0;
        virtual std::unique_ptr<ITexture> CreateTexture(int width, int height, const void* data) = 0;
        virtual std::unique_ptr<IVertexBuffer> CreateVertexBuffer() = 0;

        virtual void Clear(float r, float g, float b, float a) = 0;
        virtual void SetViewport(int x, int y, int width, int height) = 0;
        virtual void DrawTriangles(uint32_t vertexCount) = 0;
    };
}