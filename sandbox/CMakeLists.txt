add_library(sandbox_app
    main.cpp
)

target_include_directories(columnar_operations
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(sandbox_app
    PUBLIC
columnar_data_structures
columnar_core
columnar_io
 columnar_operations
 columnar_utils
)

target_compile_features(columnar_operations
    PUBLIC cxx_std_20
)
