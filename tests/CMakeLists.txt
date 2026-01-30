enable_testing()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(columnar_tests
    io/test_csv_reader.cpp
    io/test_csv_writer.cpp
    utils/test_string_converter.cpp  
    data_structures/test_data_structures.cpp  
    kt1_tests/main_test.cpp 
)

target_link_libraries(columnar_tests
    PRIVATE
        gtest_main
        columnar_core     
        columnar_io          
        columnar_data_structures 
        columnar_utils          
)

target_include_directories(columnar_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_test(NAME columnar_tests COMMAND $<TARGET_FILE:columnar_tests>)