cmake_minimum_required(VERSION 3.10)

project(quantitative_trading)

find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL 1.0.0 REQUIRED)

find_library(CPPREST_LIB cpprest)

include_directories(include)

add_executable(quantitative_trading 
    src/main.cpp
    src/websocket_client.cpp
    src/order_execution.cpp
)

target_link_libraries(quantitative_trading
    ${CPPREST_LIB}
    Boost::system
    OpenSSL::SSL
)
