cmake_minimum_required(VERSION 3.20)
project(Graphion VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(GRAPHION_ENABLE_SANITIZERS "Enable Address/Undefined sanitizers" OFF)
option(GRAPHION_ENABLE_WERROR "Treat warnings as errors" ON)
option(GRAPHION_ENABLE_ASM "Enable architecture-specific assembly hot paths" OFF)
option(GRAPHION_ENABLE_HARDENING "Enable hardening flags for non-debug builds" ON)
option(GRAPHION_ENABLE_TESTS "Build unit tests" ON)
option(GRAPHION_ENABLE_BENCHMARKS "Build benchmark binaries" ON)
option(GRAPHION_ENABLE_FUZZING "Build fuzzing target (Clang only)" OFF)

if(MSVC)
  add_compile_options(/W4)
  if(GRAPHION_ENABLE_WERROR)
    add_compile_options(/WX)
  endif()
  if(GRAPHION_ENABLE_HARDENING)
    add_compile_options("$<$<CONFIG:Release>:/guard:cf>")
    add_compile_options("$<$<CONFIG:Release>:/sdl>")
    add_link_options("$<$<CONFIG:Release>:/guard:cf>")
  endif()
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes)
  if(GRAPHION_ENABLE_WERROR)
    add_compile_options(-Werror)
  endif()
  if(GRAPHION_ENABLE_HARDENING)
    add_compile_options("$<$<NOT:$<CONFIG:Debug>>:-fstack-protector-strong>")
    add_compile_options("$<$<NOT:$<CONFIG:Debug>>:-D_FORTIFY_SOURCE=2>")
    add_compile_options("$<$<NOT:$<CONFIG:Debug>>:-fPIE>")
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:-Wl,-z,relro,-z,now>")
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:-pie>")
  endif()
endif()

if(GRAPHION_ENABLE_SANITIZERS AND NOT MSVC)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

add_library(graphion_vm
  src/vm/vm.c
  src/runtime/arena.c
  src/parser/bytecode.c
  src/graph/csr_graph.c
  src/graph/hypergraph.c
)

if(GRAPHION_ENABLE_ASM)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64|x64")
    enable_language(ASM)
    target_sources(graphion_vm PRIVATE src/vm/hotpaths.s)
    target_compile_definitions(graphion_vm PRIVATE GRAPHION_ENABLE_ASM=1)
  else()
    message(FATAL_ERROR "GRAPHION_ENABLE_ASM is only supported on x86_64/AMD64 for now.")
  endif()
endif()

target_include_directories(graphion_vm
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(graphion
  src/main.c
)

target_link_libraries(graphion PRIVATE graphion_vm)

if(GRAPHION_ENABLE_TESTS)
  enable_testing()

  add_executable(graphion_tests
    tests/test_main.c
    tests/test_vm.c
    tests/test_arena.c
    tests/test_parser.c
    tests/test_graph.c
    tests/test_hypergraph.c
  )
  target_link_libraries(graphion_tests PRIVATE graphion_vm)
  add_test(NAME graphion_tests COMMAND graphion_tests)
endif()

if(GRAPHION_ENABLE_BENCHMARKS)
  add_executable(graphion_bench
    benchmarks/bench_vm.c
  )
  target_link_libraries(graphion_bench PRIVATE graphion_vm)

  add_executable(graphion_bench_bfs
    benchmarks/bench_bfs.c
  )
  target_link_libraries(graphion_bench_bfs PRIVATE graphion_vm)

  add_executable(graphion_bench_hypergraph
    benchmarks/bench_hypergraph.c
  )
  target_link_libraries(graphion_bench_hypergraph PRIVATE graphion_vm)

  add_executable(graphion_bench_vm_graph
    benchmarks/bench_vm_graph.c
  )
  target_link_libraries(graphion_bench_vm_graph PRIVATE graphion_vm)
endif()

if(GRAPHION_ENABLE_FUZZING)
  if(MSVC)
    message(FATAL_ERROR "Fuzzing target is not supported with MSVC.")
  endif()
  if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Fuzzing requires Clang compiler.")
  endif()

  add_executable(fuzz_vm
    fuzz/fuzz_vm.c
  )
  target_compile_options(fuzz_vm PRIVATE -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer)
  target_link_options(fuzz_vm PRIVATE -fsanitize=fuzzer,address,undefined)
  target_link_libraries(fuzz_vm PRIVATE graphion_vm)

  add_executable(fuzz_parser
    fuzz/fuzz_parser.c
  )
  target_compile_options(fuzz_parser PRIVATE -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer)
  target_link_options(fuzz_parser PRIVATE -fsanitize=fuzzer,address,undefined)
  target_link_libraries(fuzz_parser PRIVATE graphion_vm)
endif()
