#include "src/memory_region_query.h"

#include <gtest/gtest.h>

namespace v8_sandbox_crash_filter {
namespace {

class VirtualMemoryFixture : public ::testing::Test {
 protected:
  void SetUp() override {
    SYSTEM_INFO info = {};
    ::GetSystemInfo(&info);
    page_size_ = info.dwPageSize;
    memory_ = ::VirtualAlloc(nullptr, page_size_, MEM_RESERVE | MEM_COMMIT,
                             PAGE_READWRITE);
    ASSERT_NE(memory_, nullptr);
  }

  void TearDown() override {
    if (memory_ != nullptr) ::VirtualFree(memory_, 0, MEM_RELEASE);
  }

  DWORD page_size_ = 0;
  void* memory_ = nullptr;
};

TEST_F(VirtualMemoryFixture, FindsCommittedReadableWritableRegion) {
  MemoryRegionInfo region = {};
  ASSERT_TRUE(QueryMemoryRegion(reinterpret_cast<std::uintptr_t>(memory_),
                                &region));
  EXPECT_EQ(region.state, MEM_COMMIT);
  EXPECT_TRUE(region.readable);
  EXPECT_TRUE(region.writable);
  EXPECT_FALSE(region.executable);
  EXPECT_TRUE(RegionAllowsAccess(region, MemoryAccessKind::kRead));
  EXPECT_TRUE(RegionAllowsAccess(region, MemoryAccessKind::kWrite));
  EXPECT_FALSE(RegionAllowsAccess(region, MemoryAccessKind::kExecute));
}

TEST_F(VirtualMemoryFixture, ReflectsReadOnlyProtection) {
  DWORD old_protection = 0;
  ASSERT_TRUE(::VirtualProtect(memory_, page_size_, PAGE_READONLY,
                               &old_protection));
  MemoryRegionInfo region = {};
  ASSERT_TRUE(QueryMemoryRegion(reinterpret_cast<std::uintptr_t>(memory_),
                                &region));
  EXPECT_TRUE(region.readable);
  EXPECT_FALSE(region.writable);
  EXPECT_FALSE(region.executable);
}

TEST_F(VirtualMemoryFixture, ReflectsExecutableReadableProtection) {
  DWORD old_protection = 0;
  ASSERT_TRUE(::VirtualProtect(memory_, page_size_, PAGE_EXECUTE_READ,
                               &old_protection));
  MemoryRegionInfo region = {};
  ASSERT_TRUE(QueryMemoryRegion(reinterpret_cast<std::uintptr_t>(memory_),
                                &region));
  EXPECT_TRUE(region.readable);
  EXPECT_FALSE(region.writable);
  EXPECT_TRUE(region.executable);
  EXPECT_TRUE(RegionAllowsAccess(region, MemoryAccessKind::kExecute));
}

TEST_F(VirtualMemoryFixture, RejectsNoAccessProtection) {
  DWORD old_protection = 0;
  ASSERT_TRUE(::VirtualProtect(memory_, page_size_, PAGE_NOACCESS,
                               &old_protection));
  MemoryRegionInfo region = {};
  ASSERT_TRUE(QueryMemoryRegion(reinterpret_cast<std::uintptr_t>(memory_),
                                &region));
  EXPECT_FALSE(region.readable);
  EXPECT_FALSE(region.writable);
  EXPECT_FALSE(region.executable);
  EXPECT_FALSE(RegionAllowsAccess(region, MemoryAccessKind::kRead));
}

TEST_F(VirtualMemoryFixture, RejectsGuardPageAsOrdinaryAccessibleMemory) {
  DWORD old_protection = 0;
  ASSERT_TRUE(::VirtualProtect(memory_, page_size_,
                               PAGE_READWRITE | PAGE_GUARD, &old_protection));
  MemoryRegionInfo region = {};
  ASSERT_TRUE(QueryMemoryRegion(reinterpret_cast<std::uintptr_t>(memory_),
                                &region));
  EXPECT_TRUE(region.guarded);
  EXPECT_FALSE(RegionAllowsAccess(region, MemoryAccessKind::kRead));
}

TEST_F(VirtualMemoryFixture, RejectsNullOutput) {
  EXPECT_FALSE(QueryMemoryRegion(reinterpret_cast<std::uintptr_t>(memory_),
                                 nullptr));
}

TEST(MemoryRegionQueryTest, ReportsReservedRegionAsNotAccessible) {
  SYSTEM_INFO info = {};
  ::GetSystemInfo(&info);
  void* memory = ::VirtualAlloc(nullptr, info.dwPageSize, MEM_RESERVE,
                                PAGE_NOACCESS);
  ASSERT_NE(memory, nullptr);

  MemoryRegionInfo region = {};
  const bool queried = QueryMemoryRegion(
      reinterpret_cast<std::uintptr_t>(memory), &region);
  EXPECT_TRUE(queried);
  if (queried) {
    EXPECT_EQ(region.state, MEM_RESERVE);
    EXPECT_FALSE(RegionAllowsAccess(region, MemoryAccessKind::kRead));
    EXPECT_FALSE(RegionAllowsAccess(region, MemoryAccessKind::kWrite));
    EXPECT_FALSE(RegionAllowsAccess(region, MemoryAccessKind::kExecute));
  }

  EXPECT_TRUE(::VirtualFree(memory, 0, MEM_RELEASE));
}

}  // namespace
}  // namespace v8_sandbox_crash_filter
