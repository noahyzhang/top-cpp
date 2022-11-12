project(top_cpp)

include_directories(
    ./src
)

file(GLOB SRC
    ./src/*.cpp
)

add_executable(top_cpp ${SRC})

target_link_libraries(top_cpp
    pthread
)
