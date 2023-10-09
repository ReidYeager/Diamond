# CMakeList.txt : CMake project for Ice, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("Diamond")

file(GLOB_RECURSE HeaderFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
file(GLOB_RECURSE HeaderPpFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB_RECURSE CSourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(GLOB_RECURSE CppSourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(Diamond ${HeaderFiles} ${HeaderPpFiles} ${CSourceFiles} ${CppSourceFiles})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Diamond PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
