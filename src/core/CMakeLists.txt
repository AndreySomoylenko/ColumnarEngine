add_library(columnar_core
    Engine.cpp
    Pipeline.cpp
    ProjectionBuilder.cpp)

target_link_libraries(columnar_core PUBLIC columnar_data_structures columnar_io)
