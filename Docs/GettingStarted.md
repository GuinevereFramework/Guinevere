# Getting Started

## Requirements

- CMake 3.20+
- A C++20 compiler
    - Windows: Visual Studio (MSVC)
- Internet access on first configure (CMake FetchContent downloads dependencies)

## Build (Windows)

From the repository root:

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

## Run demos (Windows)

```powershell
build\examples\Debug\guinevere_demo_style.exe
build\examples\Debug\guinevere_demo_window.exe
build\examples\Debug\guinevere_demo_drm.exe
build\examples\Debug\guinevere_demo_textedit.exe
```

## Use in another CMake project (single dependency)

```cmake
include(FetchContent)

FetchContent_Declare(
    guinevere
    GIT_REPOSITORY https://github.com/<your-org>/Guinevere.git
    GIT_TAG main
)
FetchContent_MakeAvailable(guinevere)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE guinevere::guinevere)
```

With this setup, your app links only `guinevere::guinevere`; GLFW/FreeType/OpenGL wiring stays inside Guinevere's CMake graph.

## Application lifecycle (recommended)

Use `guinevere::app::run(...)` as the default entrypoint. It owns:

- window creation
- renderer creation
- event pumping
- frame loop (`while(window.is_open())`)
- exception boundary (`try/catch`)

Minimal example:

```cpp
#include <guinevere/guinevere.hpp>

int main()
{
    guinevere::app::RunConfig config;
    config.width = 960;
    config.height = 640;
    config.title = "My Guinevere App";

    guinevere::app::Callbacks callbacks;
    callbacks.on_frame = [](guinevere::app::Context& context) {
        context.renderer.clear(guinevere::gfx::Color{0.08f, 0.10f, 0.13f, 1.0f});
        return true;
    };

    return guinevere::app::run(config, callbacks);
}
```

Low-level manual control (`GlfwWindow`, `create_renderer`, custom loop) remains available for advanced use cases.

For DRM code inside `app::run(...)`, prefer `UiRuntime::component_frame(...)` so your frame callback works with `ComponentScope` only:

```cpp
callbacks.on_frame = [&](guinevere::app::Context& context) {
    return ui_runtime.component_frame(
        context,
        "app",
        [](guinevere::ui::ComponentScope& component) {
            component.label("title", "Hello DRM");
        },
        guinevere::gfx::Color{0.08f, 0.10f, 0.13f, 1.0f}
    ); // continues running by default if callback returns void
};
```

## Asset Management (ID-based)

`app::Context` now exposes `assets` so you can register fonts/images once, then use IDs instead of hardcoded paths per frame.

```cpp
callbacks.on_init = [](guinevere::app::Context& context) {
    context.assets.add_search_path("fonts");
    context.assets.add_search_path("examples/assets");

    const bool font_registered = context.assets.register_font("ui", "Roboto-Bold.ttf");
    const bool image_registered = context.assets.register_image("hero", "demo-image.png");

    if(font_registered) {
        context.assets.apply_font(context.renderer, "ui", 22);
    }
    (void)image_registered;
};
```

For DRM image nodes, use `asset://` via `image_asset(...)` and pass `context.assets` to `Pipeline::paint(...)`:

```cpp
frame_builder.image_asset("root", "hero_image", "hero");
const auto commands = pipeline.build_draw_commands(tree);
pipeline.paint(context.renderer, commands, context.assets);
```

Renderer default font fallback now tries:

```text
Guinevere:/fonts/Google/Roboto-Regular.ttf
```

(`Guinevere:/` maps to the project `assets/` folder.)

Guinevere does not bundle the whole font set into your `.exe` by default. Asset files stay external and are loaded on demand only when used.

## DRM-style usage

`guinevere::ui` provides a retained tree pipeline:

- Build a list of keyed node declarations (prefer `FrameBuilder` for concise syntax).
- Compose containers with `FrameBuilder::column(...)` / `row(...)`.
- Prefer `UiRuntime` for app code:
  `component_frame(context, ...)` (component-only, one-shot), `app_frame(context, ...)` (advanced callback with `app::Context`), `frame(context)` (compact RAII), or manual `begin_frame(context)` -> build nodes via `frame_builder()` -> `end_frame(context)`.
  `end_frame(context)` does full redraw by default; pass `clear_color` to use dirty-region paint mode.
- Use `resolve_app_layout(...)` for app-level responsive container (breakpoint + margins + safe area).
- Use `resolve_app_scaffold(...)` when you want ready-to-use `header/body` regions from one app-level config.
- Prefer `UiRuntime::component_frame(context, scaffold_spec, callback)` when your frame uses scaffold every tick (auto begin/end + auto breakpoint from scaffold, no renderer access in callback).
- Prefer `UiRuntime::resolve_scaffold(context, spec)` in app code to auto-resolve from framebuffer viewport and auto-set `frame_builder().app_breakpoint(...)`.
- Use `UiRuntime::viewport(context)` when a part of your spec depends on runtime viewport size (for example, `max_height = viewport.h - 20`).
- Use `layout_in_viewport(...)` for responsive root bounds instead of hand-written viewport clamp logic.
- Use `inset_rect(...)`, `split_row_*`, and `split_column_*` helpers to derive child `Rect` regions without manual coordinate math.
- Use `axis_available_size(...)` + `resolve_axis_size(...)` to derive section heights from preferred/min/max constraints.
- Set `layout_config.main_axis_tracks` (or `Entry::main_axis_tracks(...)`) when multiple children compete on one axis and you need min/preferred/max with shrink priority and grow weight; `Pipeline::layout(...)` resolves these tracks automatically.
- If you build scaffold manually via free functions, set `frame_builder.app_breakpoint(...)` (usually from `scaffold.app_layout.breakpoint`) before using `.width({.compact=..., .expanded=...})` / `.height(...)`.
- Size children with `width_fill(weight)`, `height_fill(weight)`, `width_fixed(...)`, `height_fixed(...)`.
- Constrain bounds with `min_width(...)`, `max_width(...)`, `min_height(...)`, `max_height(...)`.
- Control layout flow with `justify_*()` and `align_*()` (or `justify(...)` / `align(...)`).
- Control overflow with `overflow_clip()`, `overflow_visible()`, `overflow_wrap()`, or `overflow_scroll()`.
- Treat UI/state keys as key *segments* (for example `panel`, `increase`, `reset`) and avoid dots in local keys; `ComponentScope`/`StateStore::Scope` now validate this.
- `ComponentScope` now auto-generates node keys when a key argument is omitted (`label(parent, text)`, `button(parent, text)`, ...). Keep explicit keys for dynamic/reordered lists where identity must follow data IDs.
- Use `ComponentScope::auto_local_key(...)` when you need explicit control (for example component mount keys or list identity).
- Duplicate node keys in the same frame are rejected with an exception instead of being silently ignored.
- For DRM component composition, prefer `mount_component(...)` / `mount_invoke(...)` with keys from `auto_local_key(...)` to avoid manual `mount(...)` + temporary `ComponentScope` boilerplate.
- Persist UI/app state across frames with `StateStore` and `scope(...)`.
- Reconcile into `UiTree` via `Reconciler::reconcile(...)`.
- Begin/end input processing via `InteractionState::begin_frame(...)` / `end_frame(...)`.
- Register button callbacks inline via `FrameBuilder` with `.on_click(...)`.
- Run `InteractionState::update_frame(tree, frame)` to auto hit-test buttons and execute callbacks.
- For scroll containers, feed `InputState.scroll_x/scroll_y` and call `update_scroll_container(node)`.
- Run `Pipeline::layout(...)` to auto-place nodes (use `layout(Rect)` only when absolute positioning is needed).
- Generate and execute draw commands with `Pipeline::build_draw_commands(...)` + `Pipeline::paint(...)`.

Low-level `Reconciler` / `InteractionState` / `Pipeline` calls are still available for advanced control.

State management pattern:

```cpp
guinevere::ui::StateStore state_store;
auto ui_state = state_store.scope("settings_panel");
bool& dark_mode = ui_state.use<bool>("dark_mode", false);
int& font_size = ui_state.use<int>("font_size", 14);
```

Reactive observer pattern:

```cpp
bool dirty = true;
auto sub = ui_state.observe("font_size", [&](const guinevere::ui::StateStore::ChangeEvent&) {
    dirty = true;
});

ui_state.update<int>("font_size", [](int& value) {
    value += 1;
}); // triggers observer
```

Use `set(...)` / `update(...)` when you need observer callbacks.

See `examples/demo_window.cpp`, `examples/demo_drm.cpp`, and `examples/demo_textedit.cpp` for retained-mode demos.

## Notes

- Dependencies are brought in with CMake `FetchContent`:
    - GLFW (window/input)
    - FreeType (font rasterization)
- OpenGL is used as the initial graphics backend for fast iteration.
- The codebase is built with warnings-as-errors.
