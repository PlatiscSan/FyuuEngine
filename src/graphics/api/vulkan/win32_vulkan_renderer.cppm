module;
#include <vulkan/vulkan.h>

export module win32_vulkan_renderer;
export import renderer_interface;
export import window_interface;
import std;

export namespace graphics::api::vulkan {

    class Win32VulkanRenderDevice : public IRenderDevice {
    private:
        VkInstance m_instance;
        VkPhysicalDevice m_physical_device;
        VkDevice m_device;
        VkQueue m_graphics_queue;
        VkCommandPool m_command_pool;
        
        static VkInstance CreateInstance() {
            VkInstance instance = nullptr;
            return instance;
        }

    public:
        Win32VulkanRenderDevice()
            : m_instance(CreateInstance()) {

        }

        ~Win32VulkanRenderDevice() noexcept override {

        }

        std::unique_ptr<IShader> CreateShader(std::filesystem::path const& vs_src, std::filesystem::path const& fs_src) override {
            return nullptr;
        }
        std::unique_ptr<ITexture> CreateTexture(int width, uint32_t height, const void* data) override {
            return nullptr;
        }
        std::unique_ptr<IVertexBuffer> CreateVertexBuffer() override {
            return nullptr;
        }

        void Clear(float r, float g, float b, float a) override {

        }
        void SetViewport(int x, int y, uint32_t width, uint32_t height) override {

        }
        void DrawTriangles(uint32_t vertex_count) override {

        }

    };

}