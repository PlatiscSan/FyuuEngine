#ifndef HASH_H
#define HASH_H

#include <cstdint>
#include <cstddef>

namespace Fyuu::utility {

	std::size_t RangeHash(std::uint32_t const* const begin, std::uint32_t const* const end, std::size_t hash);

    template <typename T> inline size_t Hash(T const* state_desc, std::size_t count = 1, std::size_t hash = 2166136261Ui64) {
        static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "State object is not word-aligned");
        return RangeHash((std::uint32_t*)state_desc, (std::uint32_t*)(state_desc + count), hash);
    }

}

#endif // !HASH_H
