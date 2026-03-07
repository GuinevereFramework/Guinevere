cmake_minimum_required(VERSION 3.20)

add_executable(guinevere_tests
    dummy_test_main.cpp
)

target_link_libraries(guinevere_tests
    PRIVATE
        guinevere
        guinevere_warnings
)

add_test(NAME guinevere_tests COMMAND guinevere_tests)

add_executable(guinevere_font_loading_tests
    font_loading_test.cpp
)

target_link_libraries(guinevere_font_loading_tests
    PRIVATE
        guinevere
        guinevere_warnings
)

add_test(NAME guinevere_font_loading_tests COMMAND guinevere_font_loading_tests)

add_executable(guinevere_asset_management_tests
    asset_management_test.cpp
)

target_link_libraries(guinevere_asset_management_tests
    PRIVATE
        guinevere
        guinevere_warnings
)

add_test(NAME guinevere_asset_management_tests COMMAND guinevere_asset_management_tests)

add_executable(guinevere_layout_helpers_tests
    layout_helpers_test.cpp
)

target_link_libraries(guinevere_layout_helpers_tests
    PRIVATE
        guinevere
        guinevere_warnings
)

add_test(NAME guinevere_layout_helpers_tests COMMAND guinevere_layout_helpers_tests)

add_executable(guinevere_text_edit_password_tests
    text_edit_password_test.cpp
)

target_link_libraries(guinevere_text_edit_password_tests
    PRIVATE
        guinevere
        guinevere_warnings
)

add_test(NAME guinevere_text_edit_password_tests COMMAND guinevere_text_edit_password_tests)
