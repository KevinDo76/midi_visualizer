cmake_minimum_required(VERSION 3.10.0)
project(midi_visualizer VERSION 0.1.0 LANGUAGES C CXX)

file(GLOB SOURCE "src/*.cpp")

add_executable(midi_visualizer ${SOURCE})

