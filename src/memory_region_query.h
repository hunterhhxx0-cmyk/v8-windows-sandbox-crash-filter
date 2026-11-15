#ifndef V8_WINDOWS_SANDBOX_CRASH_FILTER_MEMORY_REGION_QUERY_H_
#define V8_WINDOWS_SANDBOX_CRASH_FILTER_MEMORY_REGION_QUERY_H_

#ifndef _WIN32
#error "This component is Windows-only."
#endif

#include <windows.h>

#include <cstddef>
#include <cstdint>

#include "access_violation_parser.h"

namespace v8_sandbox_crash_filter {

struct MemoryRegionInfo {
  std::uintptr_t base_address;
  std::size_t region_size;
  DWORD state;
  DWORD protection;
  DWORD type;
  bool readable;
  bool writable;
  bool executable;
  bool guarded;
};

// Queries the region containing address. This calls VirtualQuery and is
// intentionally a standalone helper, not an exception-handler callback.
// Do not assume VirtualQuery is safe to call from a VEH.
inline bool QueryMemoryRegion(std::uintptr_t address, MemoryRegionInfo* out) {
  if (out == nullptr) return false;

  MEMORY_BASIC_INFORMATION mbi = {};
  const SIZE_T result =
      ::VirtualQuery(reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi));
  if (result != sizeof(mbi)) return false;

  const DWORD protection = mbi.Protect;
  const DWORD base_protection = protection & 0xff;
  const bool guarded = (protection & PAGE_GUARD) != 0;
  bool readable = false;
  bool writable = false;
  bool executable = false;

  switch (base_protection) {
    case PAGE_READONLY:
      readable = true;
      break;
    case PAGE_READWRITE:
    case PAGE_WRITECOPY:
      readable = true;
      writable = true;
      break;
    case PAGE_EXECUTE:
      executable = true;
      break;
    case PAGE_EXECUTE_READ:
      readable = true;
      executable = true;
      break;
    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
      readable = true;
      writable = true;
      executable = true;
      break;
    case PAGE_NOACCESS:
    case 0:
    default:
      break;
  }

  out->base_address = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
  out->region_size = static_cast<std::size_t>(mbi.RegionSize);
  out->state = mbi.State;
  out->protection = protection;
  out->type = mbi.Type;
  out->guarded = guarded;
  out->readable = readable && !guarded && mbi.State == MEM_COMMIT;
  out->writable = writable && !guarded && mbi.State == MEM_COMMIT;
  out->executable = executable && !guarded && mbi.State == MEM_COMMIT;
  return true;
}

inline bool RegionAllowsAccess(const MemoryRegionInfo& region,
                               MemoryAccessKind access) {
  if (region.state != MEM_COMMIT || region.guarded) return false;
  switch (access) {
    case MemoryAccessKind::kRead:
      return region.readable;
    case MemoryAccessKind::kWrite:
      return region.writable;
    case MemoryAccessKind::kExecute:
      return region.executable;
  }
  return false;
}

}  // namespace v8_sandbox_crash_filter

#endif  // V8_WINDOWS_SANDBOX_CRASH_FILTER_MEMORY_REGION_QUERY_H_
