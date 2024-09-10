
#include "pch.h"
#include "Hash.h"
#include "Memory.h"

std::size_t Fyuu::utility::RangeHash(std::uint32_t const* const begin, std::uint32_t const* const end, std::size_t hash) {

	std::uint64_t const* inter64 = (std::uint64_t const*)AlignUp(begin, 8);
	std::uint64_t const* const end64 = (std::uint64_t const*)AlignDown(end, 8);

	// If not 64-bit aligned, start with a single u32
	if ((std::uint32_t*)inter64 > begin) {
		hash = _mm_crc32_u32((std::uint32_t)hash, *begin);
	}

	// Iterate over consecutive u64 values
	while (inter64 < end64) {
		hash = _mm_crc32_u64((std::uint64_t)hash, *inter64++);
	}

	// If there is a 32-bit remainder, accumulate that
	if ((std::uint32_t*)inter64 < end) {
		hash = _mm_crc32_u32((std::uint32_t)(hash), *(std::uint32_t*)inter64);
	}

	return hash;

}
