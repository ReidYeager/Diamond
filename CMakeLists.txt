
cmake_minimum_required (VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)

file(GLOB_RECURSE HeaderFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
file(GLOB_RECURSE CSourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(GLOB_RECURSE CppSourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(diamond STATIC
  ${HeaderFiles}
  ${HeaderPpFiles}
  ${CSourceFiles}
  ${CppSourceFiles}
)

set_target_properties(diamond PROPERTIES LINKER_LANGUAGE CXX)
target_compile_definitions(diamond PUBLIC "GEM_DIAMOND")

target_include_directories (diamond PUBLIC
  ./
)

target_link_libraries(diamond
)
