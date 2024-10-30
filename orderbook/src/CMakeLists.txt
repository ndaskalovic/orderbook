add_library(OrderBookLib STATIC orderbook.cpp)

target_include_directories(OrderBookLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(OrderBookLib PUBLIC) # Add more libraries here if needed
