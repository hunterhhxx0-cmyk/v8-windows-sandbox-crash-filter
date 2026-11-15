# Integrating the Windows sandbox crash filter into V8 and d8

## Goal

Build V8's real `d8.exe` from the complete V8 source checkout, with the Windows x64 sandbox crash filter integrated into V8's existing sandbox-testing implementation. This repository is currently a standalone prototype; it is not a V8 checkout and does not currently produce `d8.exe`.

## Use the complete V8 source tree

V8's supported source checkout uses `depot_tools` and `fetch v8`, followed by `gclient sync`. Do not copy a partial selection of V8 files into this prototype repository: V8 depends on generated files, DEPS-managed third-party projects, build tooling, and platform support files.

Official instructions:
- https://v8.dev/docs/source-code
- https://v8.dev/docs/build-gn
- https://v8.dev/docs/d8

## Relevant V8 integration points (checked against upstream main)

- `src/sandbox/testing.h`: declares `SandboxTesting::Enable`, `Disable`, modes, and safe-region registration API.
- `src/sandbox/testing.cc`: currently installs the sandbox crash filter only under `V8_OS_LINUX`; non-Linux builds reach a fatal unsupported-platform path.
- `src/trap-handler/handler-outside-win.cc`: registers V8's existing Wasm VEH with `AddVectoredExceptionHandler(TRUE, HandleWasmTrap)` and removes it with `RemoveVectoredExceptionHandler`.
- `src/trap-handler/handler-inside-win.cc`: implements the Wasm VEH callback. Its comments explicitly warn that VEH code is security-sensitive, must remain self-contained, and must do very little work.
- `test/unittests/sandbox/sandbox-crash-filter-unittest.cc`: contains the existing Linux crash-filter regression tests.
- `BUILD.gn`: defines the `d8` executable and the V8 build targets.

These files are integration references, not permission to copy Linux signal-handler logic directly into a Windows VEH.

## Required integration sequence

1. Start from a full V8 checkout at a pinned revision, including DEPS dependencies.
2. Port the platform-independent classification semantics from the Linux crash filter, documenting which Linux heuristics have no safe Windows equivalent.
3. Implement Windows-specific exception classification separately. Validate exception record parameter counts and access kinds before reading them. Unknown or malformed records must continue normal exception search.
4. Design VEH coexistence with V8's existing Wasm handler, debugger first-chance handling, sanitizer handlers, handler registration order, and teardown. Do not install a second handler until this design is reviewed.
5. Keep the VEH callback minimal and auditable. Do not call `VirtualQuery`, allocate, acquire ordinary locks, log through stdio, or invoke complex V8 runtime code from the callback without a demonstrated safety argument.
6. Add the Windows implementation to V8's existing `SandboxTesting::Enable/Disable` path behind the sandbox-testing build/runtime gates. Do not change production behavior when sandbox testing is disabled.
7. Port/add Windows-specific tests in V8's sandbox crash-filter unit test target, including coexistence with Wasm trap handling, read/write/execute faults, malformed records, safe-region behavior, enable/disable lifecycle, and sanitizer/debugger behavior.
8. Build and run V8's `d8` target and relevant tests using GN/Ninja on Windows x64. The standalone CMake tests are not a substitute for this integration test.

## Expected Windows build flow

From the full V8 checkout, after installing the documented Windows prerequisites and syncing dependencies:

```bat
gn gen out.gn/x64.release --args="is_debug=false target_cpu=\"x64\" v8_enable_sandbox=true"
autoninja -C out.gn/x64.release d8
```

The exact GN arguments must be checked against the pinned V8 revision and the desired sandbox-testing configuration. The resulting shell is V8's `d8.exe`, not a wrapper around the standalone prototype.

## Current status

- Standalone Windows x64 parser and memory-region-query prototype: implemented in this repository.
- Standalone Debug and Release CI: verified passing in workflow run https://github.com/hunterhhxx0-cmyk/v8-windows-sandbox-crash-filter/actions/runs/43698578123.
- Full V8 source checkout and dependency sync: not performed here.
- V8 `testing.cc` integration: not implemented.
- Windows VEH crash filter: not implemented.
- V8 `d8.exe` build with this filter: not yet available.

Do not describe this project as an integrated Windows V8 crash filter until the V8 changes and Windows V8 tests are built and verified.
