# BridgeEngine Repository Hygiene

This repository should keep source inputs separate from local build outputs and
runtime copies. The rule is simple: Git stores the files needed to understand and
build BridgeEngine, but not generated products from a developer machine or SDK.

## Source Files

Source files are tracked in Git:

- `engine/` implementation files.
- `include/` public and internal headers.
- Root example sources such as `main.c`, `text_example.c`, `log_test.c`, and
  `xj380_main.c`.
- `Makefile`, `.clang-format`, README files, API docs, and repository docs.
- Small example assets intentionally used by examples, such as files under
  `audio_example/`, `image_example/`, `text/`, and `video_example/`.

## Vendored XJ380 SDK Inputs

The XJ380 directory is a local SDK input area, not a build output directory.
Only the parts needed for source compatibility and developer reference should be
tracked:

- `XJ380_XACT_2026v4_xj380/*.pdf`
- `XJ380_XACT_2026v4_xj380/depend/include/**`

The XJ380 GUI build also needs prebuilt SDK runtime objects. Those files are
local SDK installation artifacts and must be supplied outside Git:

- `XJ380_XACT_2026v4_xj380/depend/obj-gui/**/*.o`
- `XJ380_XACT_2026v4_xj380/depend/obj-gui/*.a`

The SDK may also include TUI runtime objects. BridgeEngine does not link them
today, but they are still local SDK artifacts and should not be tracked:

- `XJ380_XACT_2026v4_xj380/depend/obj-tui/**/*.o`
- `XJ380_XACT_2026v4_xj380/depend/obj-tui/*.a`

Place the GUI objects in the expected SDK directory before running the XJ380
targets. The Makefile reports a clear error when required GUI objects are
missing.

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
