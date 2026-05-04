add_library(columnar_io
    CSVReader.cpp
    CSVWriter.cpp
    ColumnarReader.cpp
    ColumnarWriter.cpp
    Scanner.cpp
)

target_include_directories(columnar_io
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(columnar_io 
    PUBLIC
columnar_data_structures
columnar_utils
)

target_compile_features(columnar_io
    PUBLIC cxx_std_20
)
