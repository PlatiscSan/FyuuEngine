module;
#include <vulkan/vulkan.h>

#ifdef interface
    #undef interface
#endif // interface

export module graphics:win32vulkan;
import :interface;
import window;
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

        void Clear(float r, float g, float b, float a) override {

        }
        void SetViewport(int x, int y, uint32_t width, uint32_t height) override {

        }

        bool BeginFrame() override {
            return false;
        }

        void EndFrame() override {

        }

    };

}