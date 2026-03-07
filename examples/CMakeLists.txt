cmake_minimum_required(VERSION 3.20)

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

add_executable(guinevere_demo_textedit_singleline
    demo_textedit.cpp
)

target_link_libraries(guinevere_demo_textedit_singleline
    PRIVATE
        guinevere
        guinevere_warnings
)

add_executable(guinevere_demo_textedit_multiline
    demo_textedit_multiline.cpp
)

target_link_libraries(guinevere_demo_textedit_multiline
    PRIVATE
        guinevere
        guinevere_warnings
)

add_executable(guinevere_demo_textedit_password
    demo_textedit_password.cpp
)

target_link_libraries(guinevere_demo_textedit_password
    PRIVATE
        guinevere
        guinevere_warnings
)
