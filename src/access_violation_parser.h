#ifndef V8_WINDOWS_SANDBOX_CRASH_FILTER_ACCESS_VIOLATION_PARSER_H_
#define V8_WINDOWS_SANDBOX_CRASH_FILTER_ACCESS_VIOLATION_PARSER_H_

#ifndef _WIN32
#error "This component is Windows-only."
#endif

#include <windows.h>

namespace v8_sandbox_crash_filter {

enum class MemoryAccessKind {
  kRead,
  kWrite,
  kExecute,
};

struct AccessViolationInfo {
  MemoryAccessKind access_kind;
  ULONG_PTR fault_address;
};

// Parses the documented EXCEPTION_ACCESS_VIOLATION parameters only.
// This does not determine whether the access is safe and must never be used
// by itself to swallow an exception or resume execution.
inline bool ParseAccessViolation(const EXCEPTION_RECORD& record,
                                 AccessViolationInfo* out) {
  if (out == nullptr ||
      record.ExceptionCode != EXCEPTION_ACCESS_VIOLATION ||
      record.NumberParameters < 2) {
    return false;
  }

  MemoryAccessKind kind;
  switch (record.ExceptionInformation[0]) {
    case 0:
      kind = MemoryAccessKind::kRead;
      break;
    case 1:
      kind = MemoryAccessKind::kWrite;
      break;
    case 8:
      kind = MemoryAccessKind::kExecute;
      break;
    default:
      return false;
  }

  out->access_kind = kind;
  out->fault_address =
      static_cast<ULONG_PTR>(record.ExceptionInformation[1]);
  return true;
}

}  // namespace v8_sandbox_crash_filter

#endif  // V8_WINDOWS_SANDBOX_CRASH_FILTER_ACCESS_VIOLATION_PARSER_H_
