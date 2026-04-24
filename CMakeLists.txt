cmake_minimum_required(VERSION 3.20)

project(
    alloc
    VERSION 1.0.0
    DESCRIPTION "Custom memory allocator (malloc/free/calloc/realloc)"
    LANGUAGES C
)

# ── Global settings ────────────────────────────────────────────────────────────

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Export compile_commands.json for clangd / IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release if nothing is specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

# ── Compiler warnings ──────────────────────────────────────────────────────────

add_library(alloc_warnings INTERFACE)
target_compile_options(alloc_warnings INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wcast-align
    -Wstrict-prototypes
    -Wmissing-prototypes
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)
target_link_options(alloc_warnings INTERFACE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

# ── Core library ───────────────────────────────────────────────────────────────

add_library(alloc STATIC
    src/allocator.c
    src/block.c
    src/freelist.c
)

target_include_directories(alloc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(NOT CMAKE_SKIP_INSTALL_RULES)
else()
    target_link_libraries(alloc PRIVATE alloc_warnings)
endif()

# Debug builds get extra fill-pattern instrumentation via a compile definition
target_compile_definitions(alloc PRIVATE
    $<$<CONFIG:Debug>:ALLOC_DEBUG=1>
)

# ── Install rules ──────────────────────────────────────────────────────────────

option(ALLOC_INSTALL "Install the library and headers" ON)

if(ALLOC_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    # Library + headers
    install(TARGETS alloc
        EXPORT  allocTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # CMake package config so find_package(alloc) works downstream
    install(EXPORT allocTargets
        FILE        allocTargets.cmake
        NAMESPACE   alloc::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/alloc
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/allocConfigVersion.cmake"
        VERSION       ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/allocConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/allocConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/alloc
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/allocConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/allocConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/alloc
    )
endif()
