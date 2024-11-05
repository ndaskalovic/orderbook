cmake_minimum_required(VERSION 3.10)

project(OrderBook)
set(CMAKE_CXX_STANDARD 20)

enable_testing() 

add_subdirectory(src)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_subdirectory(test)

link_directories(/usr/local/lib)

add_executable(main main.cpp)
add_executable(client client.cpp)
add_executable(server server.cpp)
add_executable(bgdClient bgdClient.cpp)

target_link_libraries(main PRIVATE OrderBookLib)
target_link_libraries(client PRIVATE OrderBookLib aeron aeron_driver aeron_client_shared pthread dl m sqlite3)
target_link_libraries(bgdClient PRIVATE OrderBookLib aeron aeron_driver aeron_client_shared pthread dl m sqlite3)
target_link_libraries(server PRIVATE OrderBookLib aeron aeron_driver aeron_client_shared pthread dl m sqlite3)
target_include_directories(client PRIVATE /usr/local/include)
target_include_directories(bgdClient PRIVATE /usr/local/include)
target_include_directories(server PRIVATE /usr/local/include)