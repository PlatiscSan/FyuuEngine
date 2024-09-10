
#include "pch.h"
#include "MemoryManager.h"
#include "Framework/Utility/Memory.h"

using namespace Fyuu::core::memory;
using namespace Fyuu::utility;

class Allocator;

static std::size_t const s_sizes[] = {

	// 4-increments
	4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48,
	52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96,

	// 32-increments
	128, 160, 192, 224, 256, 288, 320, 352, 384,
	416, 448, 480, 512, 544, 576, 608, 640,

	// 64-increments
	704, 768, 832, 896, 960, 1024

};

static std::unordered_map<std::size_t, std::unique_ptr<Allocator>> s_allocators;

class Allocator final {

public:

	Allocator(Allocator const&) = delete;
	Allocator operator=(Allocator const&) = delete;

	Allocator(std::size_t page_size, std::size_t data_size, std::size_t alignment);
	~Allocator();

	void* Allocate();
	bool Deallocate(void* ptr);

private:

	std::size_t m_page_size = 0;
	std::size_t m_data_size = 0;
	std::size_t m_alignment = 0;

	struct Page {
		Page* next;
	};

	struct Block {
		Allocator* owner;
		Block* next;
	};

	std::atomic<Page*> m_pages;
	std::atomic<Block*> m_free_blocks;

	std::uint8_t* BlockData(Block* block) const noexcept {
		return reinterpret_cast<uint8_t*>(AlignUp(reinterpret_cast<std::uintptr_t>(block + 1), m_alignment));
	}

	std::uint8_t* BlockEnd(Block* block) const noexcept {
		return reinterpret_cast<uint8_t*>(BlockData(block) + m_data_size + sizeof(Block**));
	}

	void RequestPage();

};

Allocator::Allocator(std::size_t page_size, std::size_t data_size, std::size_t alignment) 
	:m_page_size(page_size), m_data_size(data_size), m_alignment(alignment) {

	RequestPage();

}

Allocator::~Allocator() {

	Page* cur = m_pages.load();

	while (cur->next) {

		Page* page = cur;
		cur = cur->next;
		std::free(page);
		page = nullptr;

	}

	std::free(cur);
	cur = nullptr;
	m_pages = nullptr;

}

void* Allocator::Allocate() {

	if (!m_free_blocks) {
		RequestPage();
	}

	Block* block = m_free_blocks;
	m_free_blocks = m_free_blocks.load()->next;

	return BlockData(block);

}

bool Allocator::Deallocate(void* ptr) {

	if (!ptr ||
		this != (*reinterpret_cast<Block**>(static_cast<std::uint8_t*>(ptr) + m_data_size))->owner) {
		return false;
	}

	Block* blocks = *reinterpret_cast<Block**>(static_cast<std::uint8_t*>(ptr) + m_data_size);
	blocks->next = m_free_blocks;
	m_free_blocks = blocks;

	return true;

}

void Allocator::RequestPage() {

	Page* page = static_cast<Page*>(std::malloc(8192));
	if (!page) {
		throw std::bad_alloc();
	}

	if (!m_pages) {
		page->next = nullptr;
		m_pages = page;
	}
	else {
		page->next = m_pages;
		m_pages = page;
	}

	Block* free_blocks = reinterpret_cast<Block*>(AlignUp(page, m_alignment));

	
	for (Block* cur = free_blocks; BlockData(cur) + m_data_size + sizeof(Block**) <= reinterpret_cast<std::uint8_t*>(page) + m_page_size; cur = cur->next) {

		cur->owner = this;
		*reinterpret_cast<Block**>(BlockData(cur) + m_data_size) = cur;
		cur->next = reinterpret_cast<Block*>(AlignUp(BlockEnd(cur), m_alignment));
		if (reinterpret_cast<std::uint8_t*>(cur->next) > reinterpret_cast<std::uint8_t*>(page) + m_page_size) {
			cur->next = nullptr;
			break;
		}

	}

	m_free_blocks = free_blocks;

}

void* Fyuu::core::memory::MemoryAllocate(std::size_t size_in_bytes) {

	std::size_t i;
	for (i = 0; i < sizeof(s_sizes) / s_sizes[0]; i++) {
		if (size_in_bytes <= s_sizes[i]) {
			break;
		}
	}

	if (i >= sizeof(s_sizes) / s_sizes[0]) {
		return std::malloc(size_in_bytes);
	}

	if (!s_allocators.contains(s_sizes[i])) {
		s_allocators.emplace(s_sizes[i], std::make_unique<Allocator>(8192, s_sizes[i], 16));
	}

	return s_allocators[s_sizes[i]]->Allocate();

}

void Fyuu::core::memory::MemoryDeallocate(void* memory_chunck) {

	for (auto& pair : s_allocators) {
		if (pair.second->Deallocate(memory_chunck)) {
			return;
		}
	}

	std::free(memory_chunck);

}
