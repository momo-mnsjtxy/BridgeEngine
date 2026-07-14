# BridgeEngine Repository Hygiene

This repository should keep source inputs separate from local build outputs and
runtime copies. The rule is simple: Git stores the files needed to understand and
build BridgeEngine, but not generated products from a developer machine or SDK.

## Source Files

Source files are tracked in Git:

- `src/` implementation files and private headers under `src/internal/`.
- `include/BridgeEngine.h` is the sole public header; all other headers are private.
- Example sources under `examples/desktop/`, `examples/text/`, `examples/log/`,
  and `examples/xj380/`.
- `CMakeLists.txt`, CMake presets, `.clang-format`, README files, API docs, and repository docs.
- Example assets intentionally used by examples, such as files under
  `examples/assets/{audio,image,text,video}/`.

## Local XJ380 SDK

The XJ380 SDK is a local developer-provided dependency, not repository content.
Do not track `XJ380_XACT_2026v4_xj380/` in Git. Developers who need XJ380
targets should place the SDK at:

- `XJ380_XACT_2026v4_xj380/`

The XJ380 GUI build expects SDK headers and prebuilt runtime objects under that
local directory:

- `XJ380_XACT_2026v4_xj380/depend/include/**`

- `XJ380_XACT_2026v4_xj380/depend/obj-gui/**/*.o`
- `XJ380_XACT_2026v4_xj380/depend/obj-gui/*.a`

The SDK may also include PDFs, tools, EPF files, and TUI runtime objects. They
are all local SDK artifacts and should not be tracked:

- `XJ380_XACT_2026v4_xj380/*.pdf`
- `XJ380_XACT_2026v4_xj380/bin/**`
- `XJ380_XACT_2026v4_xj380/depend/obj-tui/**/*.o`
- `XJ380_XACT_2026v4_xj380/depend/obj-tui/*.a`

The CMake `check_xj380_sdk` target reports a clear error when required XJ380
headers or GUI runtime objects are missing.

## Build And Runtime Outputs

Generated files and local runtime copies are ignored by default:

- Object and archive files: `*.o`, `*.a`
- Shared libraries: `*.so`, `*.dll`
- Executables and XJ380 programs: `*.exe`, `*.epf`, `main`, `text_example`,
  `bridgeengine_demo.epf`
- BridgeEngine libraries: `libbridgeengine.so`, `libbridgeengine.dll`,
  `libbridgeengine.a`, `libbridgeengine_xj380.a`
- Logs: `*.log`

Do not add these files with `git add -f` unless a maintainer explicitly decides
that a specific binary is a source input and documents the exception here.

## Architecture Review Priority

The first maintenance priority is repository hygiene and source boundary clarity.
After that, module deepening should proceed in this order:

1. Keep the repository source boundary clean.
2. Hide platform details behind the public BAPI interface, using HE3D's backend
   separation as the portability reference point.
3. Narrow `plat_interface_t`.
4. Gather render context global state behind a smaller interface.
5. Add a clear scene/level persistence seam.
