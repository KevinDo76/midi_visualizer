cmake_minimum_required(VERSION 3.10.0)
project(midi_visualizer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SOURCE "src/*.cpp")
file(GLOB HEADER "src/*.h")


add_subdirectory(lib/fluidsynth)
add_subdirectory(lib/SDL3)

add_executable(midi_visualizer ${SOURCE} ${HEADER})   

target_link_libraries(midi_visualizer PRIVATE libfluidsynth SDL3::SDL3)