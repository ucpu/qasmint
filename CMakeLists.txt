cmake_minimum_required(VERSION 3.10)
project(qasmint CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/externals/cage/cmake")
include(cage_all)
cage_default_init()
cage_assets_add_data_path("${CMAKE_CURRENT_SOURCE_DIR}/data")

add_subdirectory(externals/cage EXCLUDE_FROM_ALL)
add_subdirectory(sources)
