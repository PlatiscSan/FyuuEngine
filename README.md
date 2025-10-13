
```
FyuuEngine
├─ asset
│  └─ shader
│     ├─ cube.hlsl
│     └─ triangle.hlsl
├─ CMakeLists.txt
├─ CMakePresets.json
├─ include
│  ├─ export_macros.h
│  ├─ fyuu_application.h
│  ├─ fyuu_logger.h
│  ├─ fyuu_rendering.h
│  ├─ glad
│  │  ├─ glad.h
│  │  └─ glad_wgl.h
│  └─ KHR
│     └─ khrplatform.h
├─ LICENSE.txt
├─ README.md
└─ src
   ├─ application
   │  └─ CMakeLists.txt
   └─ engine
      ├─ application
      │  ├─ adapter
      │  │  ├─ adapter.capplication.cppm
      │  │  ├─ adapter.capplication.impl.cpp
      │  │  ├─ adapter.clogger.cppm
      │  │  ├─ adapter.clogger.impl.cpp
      │  │  ├─ adapter.cppm
      │  │  ├─ adapter.simle_logger.cppm
      │  │  └─ adapter.simle_logger.impl.cpp
      │  ├─ dispatcher
      │  │  ├─ dispatcher.command_object.cppm
      │  │  ├─ dispatcher.command_object.impl.cpp
      │  │  └─ dispatcher.cppm
      │  ├─ fyuu_application.cpp
      │  └─ fyuu_rendering.cpp
      ├─ CMakeLists.txt
      ├─ concurrency
      │  ├─ circular_buffer.cppm
      │  ├─ concurrenct_hash_map.cppm
      │  ├─ concurrenct_hash_set.cppm
      │  ├─ concurrent_container_base.cppm
      │  ├─ concurrent_vector.cppm
      │  ├─ coroutine.cppm
      │  ├─ message_bus.cppm
      │  ├─ message_bus.impl.cpp
      │  ├─ scheduler.cppm
      │  ├─ scheduler.impl.cpp
      │  ├─ static_hash_map.cppm
      │  ├─ synchronized_function.cppm
      │  ├─ worker.cppm
      │  └─ worker.impl.cpp
      ├─ config
      │  ├─ config.cppm
      │  ├─ config.impl.cpp
      │  ├─ config.interface.cppm
      │  ├─ config.node.cppm
      │  ├─ config.node.impl.cpp
      │  ├─ config.number.cppm
      │  ├─ config.number.impl.cpp
      │  ├─ config.yaml.cppm
      │  └─ config.yaml.impl.cpp
      ├─ core
      │  ├─ rendering.command_object.cppm
      │  ├─ rendering.cppm
      │  ├─ rendering.pso.cppm
      │  ├─ rendering.renderer.cppm
      │  ├─ window.cppm
      │  ├─ window.event.cppm
      │  └─ window.interface.cppm
      ├─ logger
      │  └─ simple_logger
      │     ├─ simple_logger.core.cppm
      │     ├─ simple_logger.core.impl.cpp
      │     ├─ simple_logger.cppm
      │     ├─ simple_logger.interface.cppm
      │     ├─ simple_logger.sink.cppm
      │     └─ simple_logger.sink.impl.cpp
      ├─ platform
      │  └─ windows
      │     ├─ rendering
      │     │  ├─ directx_backend
      │     │  │  ├─ d3d12_backend.buffer.cppm
      │     │  │  ├─ d3d12_backend.buffer.impl.cpp
      │     │  │  ├─ d3d12_backend.command_object.cppm
      │     │  │  ├─ d3d12_backend.command_object.impl.cpp
      │     │  │  ├─ d3d12_backend.cppm
      │     │  │  ├─ d3d12_backend.heap_pool.cppm
      │     │  │  ├─ d3d12_backend.heap_pool.impl.cpp
      │     │  │  ├─ d3d12_backend.pso.cppm
      │     │  │  ├─ d3d12_backend.pso.impl.cpp
      │     │  │  ├─ d3d12_backend.renderer.cppm
      │     │  │  ├─ d3d12_backend.renderer.impl.cpp
      │     │  │  ├─ d3d12_backend.util.cppm
      │     │  │  └─ d3d12_backend.util.impl.cpp
      │     │  ├─ opengl_backend
      │     │  │  ├─ glad_wgl.c
      │     │  │  ├─ windows_opengl.cppm
      │     │  │  ├─ windows_opengl.renderer.cppm
      │     │  │  └─ windows_opengl.renderer.impl.cpp
      │     │  └─ vulkan_backend
      │     │     ├─ windows_vulkan.cppm
      │     │     ├─ windows_vulkan.renderer.cppm
      │     │     └─ windows_vulkan.renderer.impl.cpp
      │     ├─ win32exception.cppm
      │     ├─ win32exception.impl.cpp
      │     ├─ windows_util.cppm
      │     ├─ windows_util.impl.cpp
      │     ├─ windows_window.cppm
      │     └─ windows_window.impl.cpp
      ├─ rendering
      │  ├─ opengl_backend
      │  │  ├─ glad.c
      │  │  ├─ opengl_backend.buffer.cppm
      │  │  ├─ opengl_backend.buffer.impl.cpp
      │  │  ├─ opengl_backend.command_object.cppm
      │  │  ├─ opengl_backend.command_object.impl.cpp
      │  │  ├─ opengl_backend.cppm
      │  │  ├─ opengl_backend.pso.cppm
      │  │  ├─ opengl_backend.pso.impl.cpp
      │  │  ├─ opengl_backend.renderer.cppm
      │  │  └─ opengl_backend.renderer.impl.cpp
      │  ├─ spirv
      │  │  ├─ cross_backend
      │  │  │  ├─ cross_backend.compiler.cppm
      │  │  │  ├─ cross_backend.compiler.impl.cpp
      │  │  │  ├─ cross_backend.cppm
      │  │  │  ├─ cross_backend.reflection.cppm
      │  │  │  └─ cross_backend.reflection.impl.cpp
      │  │  ├─ reflect_backend
      │  │  │  ├─ reflect_backend.cppm
      │  │  │  ├─ reflect_backend.reflection.cppm
      │  │  │  └─ reflect_backend.reflection.impl.cpp
      │  │  ├─ spirv.cppm
      │  │  └─ spirv.interface.cppm
      │  └─ vulkan_backend
      │     ├─ vulkan_backend.buffer.cppm
      │     ├─ vulkan_backend.buffer.impl.cpp
      │     ├─ vulkan_backend.buffer_pool.cppm
      │     ├─ vulkan_backend.buffer_pool.impl.cpp
      │     ├─ vulkan_backend.command_object.cppm
      │     ├─ vulkan_backend.command_object.impl.cpp
      │     ├─ vulkan_backend.cppm
      │     ├─ vulkan_backend.pso.cppm
      │     ├─ vulkan_backend.pso.impl.cpp
      │     ├─ vulkan_backend.renderer.cppm
      │     └─ vulkan_backend.renderer.impl.cpp
      └─ utility
         ├─ collective_resource.cppm
         ├─ defer.cppm
         ├─ disable_copy.cppm
         ├─ file.cppm
         ├─ file.impl.cpp
         ├─ pointer_wrapper.cppm
         ├─ singleton.cppm
         └─ unique_allocated_object.cppm

```