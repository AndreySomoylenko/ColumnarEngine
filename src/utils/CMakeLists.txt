add_library(columnar_utils StringConverter.cpp Compresser.cpp)

target_include_directories(columnar_utils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)
