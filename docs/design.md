# Windows x64 prototype: design and safety boundaries

## Scope

This repository is an experimental companion prototype for the V8 Sandbox Crash
Filter. It is not a V8 patch and does not install an exception handler. The
first milestone is to validate Windows exception metadata parsing and memory
region inspection independently before designing integration.

## Exception metadata

For EXCEPTION_ACCESS_VIOLATION, the first two
EXCEPTION_RECORD.ExceptionInformation entries are interpreted as:

- 0: read access
- 1: write access
- 8: execute access
- entry 1: faulting virtual address

ExceptionAddress is the address of the instruction that raised the
exception; it is not the faulting data address. The parser checks the exception
code, requires at least two parameters, rejects counts greater than
EXCEPTION_MAXIMUM_PARAMETERS, and rejects unknown access kinds. The output
structure is written only after all validation succeeds.

## VirtualQuery helper

QueryMemoryRegion is a normal-context helper around VirtualQuery. It
reports the containing region's base, size, state, type, protection, and
basic read/write/execute permissions. Guard pages are treated as inaccessible
for this helper's conservative access check. Reserved and free regions do not
pass RegionAllowsAccess, which requires MEM_COMMIT.

This is not a complete security policy. In particular, region protection
cannot establish that an access is safe for the V8 sandbox, nor does it prove
that the address belongs to a benign allocation. Callers must also consider
integer overflow, region boundaries, concurrent protection changes, and the
semantics of the access being investigated. A successful query is only a
snapshot; the mapping can change immediately afterward.

**Do not call this helper from a vectored exception handler (VEH).** The
prototype has not established that VirtualQuery is safe, bounded, or
reentrancy-safe in that context. No VEH is registered by this project.

## Validation

The unit tests cover access-violation parsing, malformed metadata, committed
read/write/execute protections, no-access and guard pages, and reserved memory.
GitHub Actions builds and runs the suite in both Debug and Release on Windows
x64. A successful workflow run is still required before claiming that a given
revision has passed CI.

## Planned integration investigation

Before any handler is implemented, investigate:

1. Existing V8 Windows trap-handler registration and VEH ordering.
2. Debugger first-chance exception behavior and coexistence with other handlers.
3. Handler lifetime, unregistration, and DLL/module lifetime constraints.
4. Which filtering decisions can be made from precomputed immutable state
   without allocation, locks, or unsafe API calls.
5. Stack-overflow and safe-region policy differences from Linux.
6. Sanitizer behavior and forwarding to the prior exception search path.

Unknown, malformed, and unrelated exceptions must not be swallowed.
