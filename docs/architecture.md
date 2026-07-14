# BridgeEngine Internal Architecture

BridgeEngine keeps the public BAPI surface stable while isolating platform-specific work behind `src/internal/platform/platform.h`.

## Platform Layer

Platform adapters expose grouped capabilities:

- `core`: init, shutdown, timing, logging
- `window`: windows, events, mouse state
- `renderer`: renderer commands
- `texture`: texture and image loading
- `text`: font and text rendering
- `audio`: audio devices, streams, WAV loading
- `sync`: mutexes

`plat_get()` remains the single internal entry point. Engine modules call grouped capabilities directly; `plat_interface_t` has no flattened compatibility function pointers.

## Text Backends

The SDL3 desktop backend uses SDL3_ttf and loads font files through the BAPI text functions.

The XJ380 backend uses XAPI's built-in text drawing path. XJ380 user headers currently define `WSTR` as `char *`, so the backend treats BAPI text as byte strings passed through to XAPI; it does not perform UTF-16 or `wchar_t` conversion. XJ380 font file loading is not implemented yet, and encoding compatibility should be handled in a dedicated adapter if XAPI requires a specific non-UTF-8 encoding.

## Engine State

Single-instance rendering state lives behind `bapi_render_context()`. This does not add multi-window or multi-engine support; it collects window, renderer, and text lifecycle state that would otherwise be scattered across file-static globals.

## Events

BAPI owns its event enum and event struct. Platform adapters still produce `plat_event_t`, and `src/core/init.c` converts those into `bapi_event_t` before returning from `bapi_poll_event()`.

## Future 2.5D

Do not add 2.5D or world-space rendering to `src/draw.c`. The basic render module should remain a thin 2D drawing layer.

Future 2.5D work should live in a separate module such as `world25d` or `render25d`, with its own scene/world data model and a narrow bridge to the existing renderer capability interface.
