cmake_minimum_required(VERSION 3.20)

cmake_policy(SET CMP0135 NEW)

project(ColumnarEngine
    VERSION 0.1
    LANGUAGES CXX
)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

if (ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif()

add_compile_options(
    $<$<CONFIG:Debug>:-Wall>
    $<$<CONFIG:Debug>:-Wextra>
    $<$<CONFIG:Debug>:-Wpedantic>

    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-DNDEBUG>
    $<$<CONFIG:Release>:-march=native>
)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(sandbox)