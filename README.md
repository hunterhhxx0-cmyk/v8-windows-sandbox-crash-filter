# V8 Windows Sandbox Crash Filter

Experimental Windows x64 prototype investigating a Windows implementation of
the V8 Sandbox Crash Filter. ARM64 is a later goal, after x64 behavior is
validated.

**Status: early prototype.** This repository currently contains a conservative
access-violation metadata parser and a standalone `VirtualQuery` memory-region
inspector. It does not yet register a vectored exception handler, filter
crashes, or integrate with V8. Nothing here should be treated as production
security code.

## Build and test (Windows x64)

Requirements: Visual Studio 2022 C++ build tools, CMake 3.20+, Git, and network
access for GoogleTest FetchContent.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The same configure/build/test sequence runs in the Windows x64 GitHub Actions
workflow.

## Project layout

- `src/access_violation_parser.h`: validates and parses Windows access
  violation exception metadata.
- `src/memory_region_query.h`: queries the memory region containing an
  address using `VirtualQuery`; intended for normal execution context only.
- `test/`: GoogleTest unit tests.
- `docs/design.md`: design notes, constraints, and integration risks.

## Important safety boundary

The helpers are not a crash filter and do not decide whether an exception is
benign. In particular, do not call `VirtualQuery` from a VEH based on this
prototype. Handler registration, exception forwarding, sandbox policy, and
V8 integration require separate design and review.

See [design notes](docs/design.md) for details.
