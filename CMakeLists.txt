cmake_minimum_required(VERSION 3.20)
project(Graphion VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(GRAPHION_ENABLE_SANITIZERS "Enable Address/Undefined sanitizers" OFF)
option(GRAPHION_ENABLE_WERROR "Treat warnings as errors" ON)

if(MSVC)
  add_compile_options(/W4)
  if(GRAPHION_ENABLE_WERROR)
    add_compile_options(/WX)
  endif()
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes)
  if(GRAPHION_ENABLE_WERROR)
    add_compile_options(-Werror)
  endif()
endif()

if(GRAPHION_ENABLE_SANITIZERS AND NOT MSVC)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

add_library(graphion_vm
  src/vm/vm.c
  src/runtime/arena.c
)

target_include_directories(graphion_vm
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(graphion
  src/main.c
)

target_link_libraries(graphion PRIVATE graphion_vm)
