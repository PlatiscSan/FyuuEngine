
#include "pch.h"
#include "Framework/Application/FyuuEntry.h"
#include "D3D12ApplicationBuilder.h"
#include "VulkanApplicationBuilder.h"
#include "OpenGLApplicationBuilder.h"

using namespace Fyuu;

enum RendererAPI {

    API_NULL,
    D3D12,
    VULKAN,
    OPENGL

};

static auto InitD3D12App(int& argc, char**& argv) {

    auto builder = std::make_shared<D3D12ApplicationBuilder>();
    auto director = std::make_shared<ApplicationDirector>(builder);
    director->Construct();
    return builder->GetApp();

}

static auto InitVulkanApp(int& argc, char**& argv) {

    auto builder = std::make_shared<VulkanApplicationBuilder>();
    auto director = std::make_shared<ApplicationDirector>(builder);
    director->Construct();
    return builder->GetApp();

}

static auto InitOpenGLApp(int& argc, char**& argv) {

    auto builder = std::make_shared<OpenGLApplicationBuilder>();
    auto director = std::make_shared<ApplicationDirector>(builder);
    director->Construct();
    return builder->GetApp();

}


std::shared_ptr<Application> Fyuu::InitWindowsApp(int& argc, char**& argv) {

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

    switch (api) {
    case D3D12:
        return InitD3D12App(argc, argv);
    case VULKAN:
        return InitVulkanApp(argc, argv);
    case OPENGL:
        return InitOpenGLApp(argc, argv);
    default:
        throw std::runtime_error("Unknown renderer API selected");
    }

}
