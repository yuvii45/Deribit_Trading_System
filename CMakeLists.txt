cmake_minimum_required(VERSION 3.10)

project(quantitative_trading)

find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL 1.0.0 REQUIRED)
find_package(spdlog REQUIRED)

find_library(CPPREST_LIB cpprest)

include_directories(include)
include_directories(include/websocketpp)

add_executable(client 
    src/main.cpp
    src/websocket_client.cpp
    src/order_execution.cpp
)

add_executable(server 
    src/websocket_server.cpp
    src/websocket_client.cpp
)

target_link_libraries(client
    ${CPPREST_LIB}
    Boost::system
    OpenSSL::SSL
    spdlog::spdlog
)

target_link_libraries(server
    ${CPPREST_LIB}
    Boost::system
    OpenSSL::SSL
    spdlog::spdlog
)
