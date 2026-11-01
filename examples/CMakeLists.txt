add_executable(aa55_basic basic.c)
target_link_libraries(aa55_basic PRIVATE aa55::frame)

if(MSVC)
    target_compile_options(aa55_basic PRIVATE /W4)
else()
    target_compile_options(aa55_basic PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
    )
endif()
