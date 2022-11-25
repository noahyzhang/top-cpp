cmake_minimum_required(VERSION 3.10)
project(top_cpp)

include_directories(
    ./src
)

file(GLOB SRC
    ./src/*.cpp
    ./*.cpp
)

add_executable(top_cpp ${SRC})

target_link_libraries(top_cpp
    pthread
)
