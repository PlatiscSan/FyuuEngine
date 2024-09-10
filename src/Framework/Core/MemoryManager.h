#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <cstdint>
#include <cstddef>


namespace Fyuu::core::memory {


    void* MemoryAllocate(std::size_t size_in_bytes);
    void MemoryDeallocate(void* memory_chunck);
  

}

#endif // !MEMORY_MANAGER_H
