#include <gtest/gtest.h>
#include <Core/Memory/LinearAllocator.h>

using namespace fun;

TEST(LinearAllocator, AllocateAndUsed) {
    LinearAllocator alloc(256);

    void* p1 = alloc.Allocate(64);
    ASSERT_NE(p1, nullptr);
    EXPECT_GE(alloc.GetUsed(), 64u);

    void* p2 = alloc.Allocate(64);
    ASSERT_NE(p2, nullptr);
    EXPECT_GT(alloc.GetUsed(), 64u);
}

TEST(LinearAllocator, OverCapacityReturnsNull) {
    LinearAllocator alloc(256);

    void* p = alloc.Allocate(512);
    EXPECT_EQ(p, nullptr);
}

TEST(LinearAllocator, ResetAndReallocate) {
    LinearAllocator alloc(256);

    alloc.Allocate(64);
    alloc.Allocate(64);
    EXPECT_GT(alloc.GetUsed(), 0u);

    alloc.Reset();
    EXPECT_EQ(alloc.GetUsed(), 0u);

    void* p = alloc.Allocate(128);
    ASSERT_NE(p, nullptr);
}

TEST(LinearAllocator, Alignment) {
    LinearAllocator alloc(256);

    void* p1 = alloc.Allocate(1, alignof(int));
    ASSERT_NE(p1, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p1) % alignof(int), 0u);

    void* p2 = alloc.Allocate(1, alignof(double));
    ASSERT_NE(p2, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p2) % alignof(double), 0u);
}

TEST(LinearAllocator, Capacity) {
    LinearAllocator alloc(1024);
    EXPECT_EQ(alloc.GetCapacity(), 1024u);
}
