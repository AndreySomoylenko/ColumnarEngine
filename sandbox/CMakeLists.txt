add_executable(sandbox_app main.cpp)

file(GLOB CSV_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.csv")
foreach(CSV_FILE ${CSV_FILES})
    file(COPY ${CSV_FILE} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

target_include_directories(sandbox_app PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/data_structures
    ${CMAKE_SOURCE_DIR}/src/utils
)

target_compile_definitions(sandbox_app
    PRIVATE
        COLUMNAR_TEST_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures"
)

target_link_libraries(sandbox_app PRIVATE 
    columnar_core 
    columnar_io 
    columnar_data_structures 
    columnar_utils
)
