#pragma once
#define DECLARE_POLYMORPHIC_TYPE(NAME)	\
namespace vulkan {  \
	class Vulkan##NAME; \
}   \
\
namespace d3d12 {	\
	class D3D12##NAME;	\
}	\
\
namespace metal {	\
	class Metal##NAME;	\
}	\
\
namespace opengl {	\
	class OpenGL##NAME;	\
}	\
\
namespace webgpu {	\
	class WebGPU##NAME;	\
}

#if !defined(__APPLE__)
#define VULKAN_OPENGL_TYPES(NAME) class vulkan::Vulkan##NAME, class opengl::OpenGL##NAME
#else
#define VULKAN_OPENGL_TYPES(NAME)
#endif

#if defined(_WIN32)
#define D3D12_TYPE(NAME) , class d3d12::D3D12##NAME
#else
#define D3D12_TYPE(NAME)
#endif

#if defined(__APPLE__)
#define METAL_TYPE(NAME) , class metal::Metal##NAME
#else
#define METAL_TYPE(NAME)
#endif

#define DECLARE_POLYMORPHIC_BASE(NAME)	\
using Polymorphic##NAME##Base = plastic::utility::PolymorphicBase<\
	VULKAN_OPENGL_TYPES(NAME)\
	D3D12_TYPE(NAME)\
	METAL_TYPE(NAME)\
	, webgpu::WebGPU##NAME\
>

#define DECLARE_POLYMORPHIC(NAME) \
DECLARE_POLYMORPHIC_TYPE(NAME)	\
DECLARE_POLYMORPHIC_BASE(NAME);