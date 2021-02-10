cmake_minimum_required(VERSION 3.10)

message(STATUS "----------------------------------------------------------------")
message(STATUS "qasmint")
message(STATUS "----------------------------------------------------------------")

########
# LIB
########

file(GLOB_RECURSE qasm-sources "libqasm/*" "include/qasm/*")
add_library(qasm STATIC ${qasm-sources})
target_include_directories(qasm PUBLIC include)
target_link_libraries(qasm PUBLIC cage-core)
cage_ide_category(qasm qasm)
cage_ide_sort_files(qasm)

########
# TESTS
########

file(GLOB_RECURSE qasm-tests-sources "tests/*")
add_executable(qasm-tests ${qasm-tests-sources})
target_link_libraries(qasm-tests qasm)
cage_ide_category(qasm-tests qasm)
cage_ide_sort_files(qasm-tests)
cage_ide_working_dir_in_place(qasm-tests)

########
# APPS
########

file(GLOB_RECURSE qasmint-sources "qasmint/*")
add_executable(qasmint ${qasmint-sources})
target_link_libraries(qasmint qasm)
cage_ide_category(qasmint qasm)
cage_ide_sort_files(qasmint)
cage_ide_working_dir_in_place(qasmint)

file(GLOB_RECURSE imgmod-sources "imgmod/*")
add_executable(imgmod ${imgmod-sources})
target_link_libraries(imgmod qasm)
cage_ide_category(imgmod qasm)
cage_ide_sort_files(imgmod)
cage_ide_working_dir_in_place(imgmod)
