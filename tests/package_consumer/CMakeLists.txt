cmake_minimum_required(VERSION 3.20)
project(BridgeEnginePackageConsumer LANGUAGES C CXX)

find_package(BridgeEngine CONFIG REQUIRED)

add_executable(consumer_shared shared.c)
target_link_libraries(consumer_shared PRIVATE BridgeEngine::bridgeengine_shared)

add_executable(consumer_static static.cpp)
target_link_libraries(consumer_static PRIVATE BridgeEngine::bridgeengine_static)
