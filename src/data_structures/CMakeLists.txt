add_library(columnar_data_structures
Column.cpp
Butch.cpp
Scheme.cpp
)

target_include_directories(columnar_data_structures
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_features(columnar_data_structures
    PUBLIC cxx_std_20
)
