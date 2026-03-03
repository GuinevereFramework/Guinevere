cmake_minimum_required(VERSION 3.20)

add_library(guinevere)
add_library(guinevere::guinevere ALIAS guinevere)

target_sources(guinevere
    PRIVATE
        asset/asset_manager.cpp
        app/runner.cpp
        style/style_engine.cpp
        ui/layout.cpp
        ui/tree.cpp
        ui/state_store.cpp
        ui/interaction.cpp
        ui/pipeline.cpp
        ui/runtime.cpp
        platform/glfw_window.cpp
        text/freetype_library.cpp
        gfx/renderer_factory.cpp
        gfx/opengl_renderer.cpp
)

target_include_directories(guinevere
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(guinevere
    PUBLIC
        guinevere_warnings
        glfw
        OpenGL::GL
        freetype
)

if(WIN32)
    target_link_libraries(guinevere PUBLIC gdiplus)
endif()
