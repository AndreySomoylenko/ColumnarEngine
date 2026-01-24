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
)

target_link_libraries(columnar_tests
    PRIVATE
        gtest_main
        columnar_io
)

add_test(NAME columnar_tests COMMAND $<TARGET_FILE:columnar_tests>)
