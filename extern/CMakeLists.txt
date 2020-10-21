# yalr/extern

cmake_minimum_required(VERSION 3.13)

add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE doctest/)

add_library(inja INTERFACE)
target_include_directories(inja INTERFACE inja/)

add_library(nlohmann-json INTERFACE)
target_include_directories(nlohmann-json INTERFACE nlohmann/)

add_library(cxxopts INTERFACE)
target_include_directories(cxxopts INTERFACE cxxopts/)
