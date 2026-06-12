add_library(columnar_operations
    Operations.cpp
)

target_include_directories(columnar_operations
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(columnar_operations
    PUBLIC
columnar_data_structures
columnar_core
)

target_compile_features(columnar_operations
    PUBLIC cxx_std_20
)
