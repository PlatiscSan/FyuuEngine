#pragma once
#include "glad/glad.h"
#if defined(_WIN32)
#include "glad/glad_wgl.h"
#elif defined(__linux__) && !defined(__ANDROID__)
#include "glad/glad_glx.h"
#elif defined(__linux__) || defined(__ANDROID__)
#include "glad/glad_egl.h"
#endif // defined(_WIN32)
