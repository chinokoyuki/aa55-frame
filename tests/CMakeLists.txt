option(AA55_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_executable(aa55_frame_tests test_aa55_frame.c)
target_link_libraries(aa55_frame_tests PRIVATE aa55::frame)

if(MSVC)
    target_compile_options(aa55_frame_tests PRIVATE /W4)
else()
    target_compile_options(aa55_frame_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(AA55_ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(aa55_frame_tests PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer
    )
    target_link_options(aa55_frame_tests PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer
    )
endif()

add_test(NAME aa55_frame_tests COMMAND aa55_frame_tests)
