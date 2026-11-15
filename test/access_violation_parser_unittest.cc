#include "src/access_violation_parser.h"

#include <gtest/gtest.h>

namespace v8_sandbox_crash_filter {
namespace {

TEST(AccessViolationParserTest, ParsesReadWriteAndExecute) {
  const ULONG_PTR kinds[] = {0, 1, 8};
  const MemoryAccessKind expected[] = {MemoryAccessKind::kRead,
                                        MemoryAccessKind::kWrite,
                                        MemoryAccessKind::kExecute};
  for (int i = 0; i < 3; ++i) {
    EXCEPTION_RECORD record = {};
    record.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
    record.NumberParameters = 2;
    record.ExceptionInformation[0] = kinds[i];
    record.ExceptionInformation[1] = 0x12345678;

    AccessViolationInfo info = {};
    ASSERT_TRUE(ParseAccessViolation(record, &info));
    EXPECT_EQ(info.access_kind, expected[i]);
    EXPECT_EQ(info.fault_address, static_cast<ULONG_PTR>(0x12345678));
  }
}

TEST(AccessViolationParserTest, RejectsOtherExceptionCodes) {
  EXCEPTION_RECORD record = {};
  record.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
  record.NumberParameters = 2;
  AccessViolationInfo info = {};
  EXPECT_FALSE(ParseAccessViolation(record, &info));
}

TEST(AccessViolationParserTest, RejectsTruncatedParameters) {
  EXCEPTION_RECORD record = {};
  record.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
  record.NumberParameters = 1;
  AccessViolationInfo info = {};
  EXPECT_FALSE(ParseAccessViolation(record, &info));
}

TEST(AccessViolationParserTest, RejectsUnknownAccessKind) {
  EXCEPTION_RECORD record = {};
  record.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
  record.NumberParameters = 2;
  record.ExceptionInformation[0] = 2;
  AccessViolationInfo info = {};
  EXPECT_FALSE(ParseAccessViolation(record, &info));
}

TEST(AccessViolationParserTest, RejectsNullOutput) {
  EXCEPTION_RECORD record = {};
  record.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
  record.NumberParameters = 2;
  EXPECT_FALSE(ParseAccessViolation(record, nullptr));
}

TEST(AccessViolationParserTest, UsesFaultAddressNotInstructionAddress) {
  EXCEPTION_RECORD record = {};
  record.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
  record.ExceptionAddress = reinterpret_cast<void*>(0x1111);
  record.NumberParameters = 2;
  record.ExceptionInformation[0] = 0;
  record.ExceptionInformation[1] = 0x2222;

  AccessViolationInfo info = {};
  ASSERT_TRUE(ParseAccessViolation(record, &info));
  EXPECT_EQ(info.fault_address, static_cast<ULONG_PTR>(0x2222));
}

}  // namespace
}  // namespace v8_sandbox_crash_filter
