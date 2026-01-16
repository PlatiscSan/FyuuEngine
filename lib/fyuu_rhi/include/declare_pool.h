#pragma once

#define DECLARE_POOL(NAME, TYPE, COUNT) \
static constexpr std::size_t NAME##_pool_size = COUNT; \
std::array<std::byte, NAME##_pool_size * sizeof(TYPE)> NAME##_buffer; \
std::pmr::monotonic_buffer_resource NAME##_pool(NAME##_buffer.data(), NAME##_buffer.size()); \
std::pmr::polymorphic_allocator<TYPE> NAME##_alloc(&NAME##_pool);