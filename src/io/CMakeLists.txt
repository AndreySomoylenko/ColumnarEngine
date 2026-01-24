add_library(columnar_io
    CSVReader.cpp
    CSVWriter.cpp
)

target_include_directories(columnar_io
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_features(columnar_io
    PUBLIC cxx_std_20
)
