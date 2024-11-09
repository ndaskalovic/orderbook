set(CMAKE_CXX_STANDARD 20)
add_library(OrderBookLib STATIC orderBook.cpp)

target_include_directories(OrderBookLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(OrderBookLib PUBLIC)
