#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>

namespace fun {

class LinearAllocator {
public:
    explicit LinearAllocator(size_t capacity)
        : m_capacity(capacity)
        , m_used(0)
        , m_buffer(static_cast<uint8_t*>(std::malloc(capacity))) {
    }

    ~LinearAllocator() {
        std::free(m_buffer);
    }

    // 禁止拷贝
    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

    // 允许移动
    LinearAllocator(LinearAllocator&& other) noexcept
        : m_capacity(other.m_capacity)
        , m_used(other.m_used)
        , m_buffer(other.m_buffer) {
        other.m_buffer = nullptr;
        other.m_capacity = 0;
        other.m_used = 0;
    }

    LinearAllocator& operator=(LinearAllocator&& other) noexcept {
        if (this != &other) {
            std::free(m_buffer);
            m_buffer = other.m_buffer;
            m_capacity = other.m_capacity;
            m_used = other.m_used;
            other.m_buffer = nullptr;
            other.m_capacity = 0;
            other.m_used = 0;
        }
        return *this;
    }

    void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        if (!m_buffer) return nullptr;

        // 对齐当前偏移
        uintptr_t current = reinterpret_cast<uintptr_t>(m_buffer + m_used);
        uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
        size_t padding = aligned - current;

        if (m_used + padding + size > m_capacity) {
            return nullptr;
        }

        m_used += padding + size;
        return reinterpret_cast<void*>(aligned);
    }

    void Reset() {
        m_used = 0;
    }

    size_t GetUsed() const { return m_used; }
    size_t GetCapacity() const { return m_capacity; }

private:
    size_t m_capacity;
    size_t m_used;
    uint8_t* m_buffer;
};

} // namespace fun
