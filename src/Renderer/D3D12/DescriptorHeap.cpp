#include "pch.h"
#include "DescriptorHeap.h"
#include "CommandListManager.h"

using namespace Fyuu::utility;
using namespace Fyuu::graphics::d3d12;

static std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> s_descriptor_heap_pool;
static std::mutex s_pool_mutex;
constexpr std::size_t NUM_DESCRIPTOR_PER_HEAP = 256ui64;

static std::array<DescriptorAllocator, 4> s_descriptor_allocator = {
    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
    D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
    D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
    D3D12_DESCRIPTOR_HEAP_TYPE_DSV
};


static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) {

    std::lock_guard<std::mutex> lock(s_pool_mutex);

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = type;
    desc.NumDescriptors = NUM_DESCRIPTOR_PER_HEAP;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 1;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
    ThrowIfFailed(D3D12Device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
    s_descriptor_heap_pool.emplace_back(heap);

    return heap;

}

Fyuu::graphics::d3d12::DescriptorHeap::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, std::uint32_t max_count) {

    m_heap_desc.Type = type;
    m_heap_desc.NumDescriptors = max_count;
    m_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    m_heap_desc.NodeMask = 1;

    ThrowIfFailed(D3D12Device()->CreateDescriptorHeap(&m_heap_desc, IID_PPV_ARGS(&m_heap)));

    m_descriptor_size = D3D12Device()->GetDescriptorHandleIncrementSize(m_heap_desc.Type);
    m_num_free_descriptors = m_heap_desc.NumDescriptors;
    m_first_handle = DescriptorHandle(m_heap->GetCPUDescriptorHandleForHeapStart(), m_heap->GetGPUDescriptorHandleForHeapStart());
    m_next_free_handle = m_first_handle;


}

DescriptorHandle Fyuu::graphics::d3d12::DescriptorHeap::Allocate(std::uint32_t count) {

    if (!HasAvailableSpace(count)) {
        throw std::runtime_error("Descriptor Heap out of space.");
    }

    DescriptorHandle ret = m_next_free_handle;
    m_next_free_handle += (count * m_descriptor_size);
    m_next_free_handle -= count;

    return ret;

}

bool Fyuu::graphics::d3d12::DescriptorHeap::ValidateHandle(DescriptorHandle const& handle) const noexcept {

    if (handle.GetCPUPtr() < m_first_handle.GetCPUPtr() ||
        handle.GetCPUPtr() >= m_first_handle.GetCPUPtr() + static_cast<std::size_t>(m_heap_desc.NumDescriptors) * m_descriptor_size) {
        return false;
    }

    if (handle.GetGPUPtr() - m_first_handle.GetGPUPtr() != handle.GetCPUPtr() - m_first_handle.GetCPUPtr()) {
        return false;
    }

    return true;

}

D3D12_CPU_DESCRIPTOR_HANDLE Fyuu::graphics::d3d12::DescriptorAllocator::Allocate(std::size_t count) {


    if (!m_current_heap || m_remaining_free_handles < count) {

        m_current_heap = RequestNewHeap(m_type);
        m_current_handle = m_current_heap->GetCPUDescriptorHandleForHeapStart();
        m_remaining_free_handles = NUM_DESCRIPTOR_PER_HEAP;

        if (!m_descriptor_size) {
            m_descriptor_size = D3D12Device()->GetDescriptorHandleIncrementSize(m_type);
        }

    }

    D3D12_CPU_DESCRIPTOR_HANDLE ret = m_current_handle;
    m_current_handle.ptr += count * m_descriptor_size;
    m_remaining_free_handles -= count;

    return ret;

}

void Fyuu::graphics::d3d12::DestroyDescriptorAllocators() {

    s_descriptor_heap_pool.clear();

}

D3D12_CPU_DESCRIPTOR_HANDLE Fyuu::graphics::d3d12::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, std::size_t count) {

    return s_descriptor_allocator[type].Allocate(count);

}
