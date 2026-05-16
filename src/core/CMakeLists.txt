add_library(columnar_core
    Engine.cpp
    Pipeline.cpp
    ProjectionBuilder.cpp)

target_include_directories(columnar_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(columnar_core PUBLIC columnar_data_structures columnar_io)
