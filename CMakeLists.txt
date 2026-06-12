cmake_minimum_required(VERSION 3.16)

project(DominantColor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)
find_package(simpleble REQUIRED CONFIG)

add_executable(
		getColor
    src/cap.cpp
    src/ble.cpp
)

target_link_libraries(
    getColor
    PRIVATE
    simpleble::simpleble
    ${OpenCV_LIBS}
)
