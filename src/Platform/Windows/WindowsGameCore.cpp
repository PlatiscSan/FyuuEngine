
#include "pch.h"
#include "Framework/Core/GameCore.h"
#include "Framework/Core/Logger.h"
#include "Framework/Core/MessageBus.h"
#include "Framework/Event/Event.h"

#include "Renderer/D3D12/D3D12Core.h"
#include "Renderer/D3D12/CommandListManager.h"

#include "Renderer/OpenGL/OpenGLCore.h"

#include "WindowsWindow.h"

using namespace Fyuu::core;
using namespace Fyuu::core::log;
using namespace Fyuu::core::message_bus;

using namespace Fyuu::graphics::d3d12;
using namespace Fyuu::graphics::opengl;

enum RendererAPI {

    API_NULL,
    D3D12,
    VULKAN,
    OPENGL

};

static void InitializeD3D12(int argc, char** argv) {

    InitializeD3D12Device();
    InitializeCommandListManager();

}

static void InitializeVulkan(int argc, char** argv) {

}

static void InitializeOpenGL(int argc, char** argv) {
    LoadWGLExt();
    InitializeWindowsGLContext(std::dynamic_pointer_cast<WindowsWindow>(MainWindow())->GetWindowHandle());
}

void Fyuu::core::InitializeWindowsApp(int argc, char** argv) {

    RendererAPI api{};
    for (std::size_t i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--d3d12") == 0) {
            if (api != API_NULL) {
                throw std::runtime_error("Cannot specify multiple renderers (--dx12, --vulkan, --opengl)");
            }
            api = D3D12;
        }
        if (std::strcmp(argv[i], "--vulkan") == 0) {
            if (api != API_NULL) {
                throw std::runtime_error("Cannot specify multiple renderers (--dx12, --vulkan, --opengl)");
            }
            api = VULKAN;
        }
        if (std::strcmp(argv[i], "--opengl") == 0) {
            if (api != API_NULL) {
                throw std::runtime_error("Cannot specify multiple renderers (--dx12, --vulkan, --opengl)");
            }
            api = OPENGL;
        }
    }

    if (api == API_NULL) {
        api = D3D12;
    }

    SetMainWindow(std::make_shared<WindowsWindow>("Fyuu Engine"));
    SetMessageLoop(
        []() {
            
            MSG msg = { 0 };
            bool quit = false;
            do {

                while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                    if (msg.message == WM_QUIT) {
                        quit = true;
                    }
                }

                if (quit) {
                    break;
                }


            } while (UpdateApplication());

        }
    );

    Subscribe<WindowDestroyEvent>(
        [](WindowDestroyEvent const& e) {
            if (e.GetWindow() == MainWindow().get()) {
                RequestQuit();
            }
        }
    );


    switch (api) {
    case D3D12:
        InitializeD3D12(argc, argv);
        break;
    case VULKAN:
        InitializeVulkan(argc, argv);
        break;
    case OPENGL:
        InitializeOpenGL(argc, argv);
        break;
    default:
        throw std::runtime_error("Unknown renderer API selected");
    }


}