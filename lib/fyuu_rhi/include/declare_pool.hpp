#pragma once

#include <version>

#include <array>
#include <vector>
#include <memory_resource>

#if defined(__cpp_lib_inplace_vector)
#include <inplace_vector>
#else
#include <boost/container/static_vector.hpp>
#endif // defined(__cpp_lib_inplace_vector)

#define DECLARE_POOL(NAME, TYPE, COUNT) \
static constexpr std::size_t NAME##_pool_size = COUNT; \
std::array<std::byte, NAME##_pool_size * sizeof(TYPE)> NAME##_buffer; \
std::pmr::monotonic_buffer_resource NAME##_pool{ NAME##_buffer.data(), NAME##_buffer.size() }; \
std::pmr::polymorphic_allocator<TYPE> NAME##_alloc{ &NAME##_pool };

