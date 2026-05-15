enable_testing()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(columnar_tests
    core/test_engine.cpp
    core/test_hits_e2e.cpp
    data_structures/test_byte_vector.cpp
    data_structures/test_columns_scheme_butch.cpp
    io/test_columnar_io.cpp
    io/test_csv_io.cpp
    operations/test_aggregations.cpp
    utils/test_hits_columns.cpp
    utils/test_string_converter.cpp
)

target_link_libraries(columnar_tests
    PRIVATE
        gtest_main
        columnar_core     
        columnar_io          
        columnar_data_structures 
        columnar_operations
        columnar_utils    
)

target_include_directories(columnar_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_compile_definitions(columnar_tests
    PRIVATE
        COLUMNAR_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

add_test(NAME columnar_tests COMMAND $<TARGET_FILE:columnar_tests>)
