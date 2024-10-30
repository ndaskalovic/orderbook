include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(OrderBookTest OrderBookTest.cpp)

# target_include_directories(OrderBookTest PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(OrderBookTest PRIVATE OrderBookLib gtest gtest_main pthread)

add_test(NAME OrderBookTest COMMAND OrderBookTest)
