cmake_minimum_required(VERSION 3.20)

add_executable(guinevere_demo_style
    demo_style.cpp
)

target_link_libraries(guinevere_demo_style
    PRIVATE
        guinevere
        guinevere_warnings
)

add_executable(guinevere_demo_window
    demo_window.cpp
)

target_link_libraries(guinevere_demo_window
    PRIVATE
        guinevere
        guinevere_warnings
)

add_executable(guinevere_demo_drm
    demo_drm.cpp
)

target_link_libraries(guinevere_demo_drm
    PRIVATE
        guinevere
        guinevere_warnings
)
