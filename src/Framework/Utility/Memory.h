#ifndef MEMORY_H
#define MEMORY_H

namespace Fyuu::utility {

    template <typename T> inline T AlignUp(T value, size_t alignment) {
        return (T)(((size_t)value + alignment) & ~alignment);
    }

    template <typename T> inline T AlignDown(T value, size_t alignment) {
        return (T)((size_t)value & ~alignment);
    }

}

#endif // !MEMORY_H
