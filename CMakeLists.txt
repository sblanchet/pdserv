cmake_minimum_required(VERSION 2.6)
project(RTLAB)

enable_testing()
include(CPack)

# Search for mercurial, used to add ID information to the library
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
find_package(Mercurial REQUIRED)
mercurial_hg_info(${PROJECT_SOURCE_DIR} Package)

add_subdirectory(src)
add_subdirectory(test)
