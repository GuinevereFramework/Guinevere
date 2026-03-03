# Architecture (DRM)

Guinevere is currently in an MVP state. The goal is to keep upper layers (UI/widgets) independent from the rendering backend.

## Modules

- `app`
    - High-level application lifecycle entrypoint (`guinevere::app::run`).
    - Owns default window/renderer bootstrapping, frame loop, and exception boundary.
    - Keeps low-level APIs available for advanced custom loops.
- `platform`
    - Window creation and event pumping.
    - Current implementation: `GlfwWindow`.
- `gfx`
    - Backend-independent drawing API.
    - Current backend: OpenGL (MVP).
- `text`
    - Font library wrappers.
    - Current implementation: FreeType RAII wrapper.
- `asset`
    - Centralized asset registry + URI resolver (`asset://font/<id>`, `asset://image/<id>`).
    - Supports search paths and ID-based lookup so apps avoid manual file-path wiring per frame.
- `style`
    - CSS-like style system.
    - Selector matching + cascade + computed properties.
- `ui`
    - Declarative Retained Mode (DRM) under `guinevere::ui`.
    - UI owns a retained `UiTree`, reconciliation, interaction state, layout, and paint-command generation.

## Rendering backend strategy

Upper layers call `guinevere::gfx::Renderer` methods. They do not include or call OpenGL directly.

A backend implementation lives under `src/gfx/` and is created via `guinevere::gfx::create_renderer(...)`.

### Current `Renderer` primitives

- `clear(Color)`
- `fill_rect(Rect, Color)`
- `stroke_rect(Rect, Color, thickness)`
- `push_clip(Rect)` / `pop_clip()`
- `set_font(ttf_path, pixel_height)`
- `draw_text(x, y, text, Color)`
- `draw_image(rect, image_path, tint)`
- `present()`

### Clip/scissor

The renderer maintains a clip stack. `push_clip` intersects the requested rectangle with the current clip (if any) and applies it.

OpenGL implementation uses `GL_SCISSOR_TEST` + `glScissor`, converting from UI coordinates (top-left origin) to OpenGL scissor coordinates (bottom-left origin).

### Text (MVP)

Text is implemented as:

- FreeType loads a font face and renders glyph bitmaps.
- A glyph atlas is stored in a single OpenGL texture.
- `draw_text` decodes UTF-8 into Unicode codepoints, ensures glyphs exist in the atlas (on-demand), and draws textured quads using glyph metrics (bearing/advance).
- Font resources are cached by `(ttf_path, pixel_height)` and evicted automatically when unused for multiple frames.
- Glyph atlas textures are released together with their cached font resource.

This is intentionally minimal. Future iterations should add shaping, better caching/atlas growth, and GPU batching.

## UI Strategy (DRM)

Guinevere uses a retained UI pipeline:

- App code describes a retained tree (`UiTree`) of keyed nodes.
- `Reconciler` rebuilds the frame tree while preserving keyed interaction state.
- `InteractionState` maps input frames to per-node widget state transitions (hover/pressed/click).
- `Pipeline` runs layout and emits backend-independent draw commands.
- Draw commands are executed through `guinevere::gfx::Renderer`.
- Draw commands can resolve asset URIs through `guinevere::asset::AssetManager` during paint.

### Current DRM scope

- Tree node kinds: `Root`, `Panel`, `View`, `Image`, `Label`, `Button`, `Custom`.
- Keyed state retention across reconciliation.
- Typed state container (`StateStore`) with scoped keys and reactive observers.
- Input frame model via `InputState`.
- Button interactions via `InteractionState`, including declarative `.on_click(...)` handlers from `FrameBuilder` processed by `update_frame(...)`.
- Built-in auto-layout (column/row flow, padding/gap, auto/fill/fixed sizing with fill weights, min/max constraints, align/justify, overflow clip/visible/wrap/scroll) with optional explicit layout hints.
- Paint command generation + renderer playback.
- Dirty-flag invalidation (`layout`/`paint`/`resource`) per node.
- Partial repaint via dirty regions (incremental clip + paint only on invalidated regions).

### Planned DRM next steps

- Event routing (capture/bubble), focus system, keyboard input, and shortcuts.
- Better layout model (constraints/flex/grid).
- Style engine integration with DRM nodes (selectors + computed style application).
- Animation/timing and partial redraw/batching.
