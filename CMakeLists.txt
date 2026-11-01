cmake_minimum_required(VERSION 3.15)

project(aa55_frame VERSION 1.0.0 LANGUAGES C)

add_library(aa55_frame INTERFACE)
add_library(aa55::frame ALIAS aa55_frame)
set_target_properties(aa55_frame PROPERTIES EXPORT_NAME frame)
target_compile_features(aa55_frame INTERFACE c_std_99)
target_include_directories(
    aa55_frame
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    option(AA55_BUILD_EXAMPLES "Build aa55-frame examples" ON)
else()
    option(AA55_BUILD_EXAMPLES "Build aa55-frame examples" OFF)
endif()

if(AA55_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

include(CTest)
if(BUILD_TESTING AND CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(AA55_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/aa55_frame")
configure_package_config_file(
    cmake/aa55_frame_config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/aa55_frame_config.cmake"
    INSTALL_DESTINATION "${AA55_CMAKE_INSTALL_DIR}"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/aa55_frame_config_version.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS aa55_frame EXPORT aa55_frame_targets)
install(
    EXPORT aa55_frame_targets
    NAMESPACE aa55::
    DESTINATION "${AA55_CMAKE_INSTALL_DIR}"
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/aa55_frame_config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/aa55_frame_config_version.cmake"
    DESTINATION "${AA55_CMAKE_INSTALL_DIR}"
)
