 # Guinevere

 Guinevere is a C++ GUI framework (Apache 2.0) targeting a modern architecture with a backend-independent rendering API.

 ## Status

 This repository is currently an MVP.

 ## Documentation

 - `Docs/GettingStarted.md`
 - `Docs/Architecture.md`

## CMake Target

Link applications against `guinevere::guinevere` (single framework target).

Guinevere runtime UI is Declarative Retained Mode (DRM) under `guinevere::ui`, with built-in layout and typed reactive state management via `StateStore`.

Default app lifecycle entrypoint is `guinevere::app::run(...)`, which wraps window/renderer setup and the main frame loop.

Recent DRM/runtime optimizations include dirty-marking for incremental layout, partial repaint using dirty regions, cached font atlas resources with automatic eviction when unused, and built-in `AssetManager` for ID-based font/image registration.

Asset files are not embedded into the executable by default; fonts/images are loaded lazily when your app references them.

