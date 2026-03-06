cmake_minimum_required(VERSION 3.20)
project(Graphion VERSION 0.1.0 LANGUAGES C)

include(CheckIPOSupported)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(GRAPHION_ENABLE_SANITIZERS "Enable Address/Undefined sanitizers" OFF)
option(GRAPHION_ENABLE_WERROR "Treat warnings as errors" ON)
option(GRAPHION_ENABLE_ASM "Enable architecture-specific assembly hot paths" OFF)
set(GRAPHION_VM_DISPATCH "switch" CACHE STRING "VM dispatch strategy: switch, jumptable, computed-goto")
set_property(CACHE GRAPHION_VM_DISPATCH PROPERTY STRINGS switch jumptable computed-goto)
option(GRAPHION_ENABLE_HARDENING "Enable hardening flags for non-debug builds" ON)
option(GRAPHION_ENABLE_IPO "Enable IPO/LTO in release-like builds when supported" ON)
option(GRAPHION_ENABLE_TESTS "Build unit tests" ON)
option(GRAPHION_ENABLE_BENCHMARKS "Build benchmark binaries" ON)
option(GRAPHION_ENABLE_FUZZING "Build fuzzing target (Clang only)" OFF)
set(GRAPHION_PGO_MODE "OFF" CACHE STRING "Profile-guided optimization mode: OFF, GENERATE, USE")
set_property(CACHE GRAPHION_PGO_MODE PROPERTY STRINGS OFF GENERATE USE)
set(GRAPHION_PGO_PROFILE_DIR "${CMAKE_BINARY_DIR}/pgo-data" CACHE PATH "Directory used to store PGO profile data")
set(GRAPHION_PGO_CLANG_PROFDATA "${GRAPHION_PGO_PROFILE_DIR}/graphion.profdata" CACHE FILEPATH
    "Merged Clang profile data file used when GRAPHION_PGO_MODE=USE")

if(NOT GRAPHION_PGO_MODE STREQUAL "OFF" AND
   NOT GRAPHION_PGO_MODE STREQUAL "GENERATE" AND
   NOT GRAPHION_PGO_MODE STREQUAL "USE")
  message(FATAL_ERROR "Invalid GRAPHION_PGO_MODE='${GRAPHION_PGO_MODE}'. Use OFF, GENERATE, or USE.")
endif()

function(graphion_enable_pgo target_name)
  if(GRAPHION_PGO_MODE STREQUAL "OFF")
    return()
  endif()

  file(TO_CMAKE_PATH "${GRAPHION_PGO_PROFILE_DIR}" graphion_pgo_profile_dir_cmake)
  file(TO_CMAKE_PATH "${GRAPHION_PGO_CLANG_PROFDATA}" graphion_pgo_profdata_cmake)

  if(MSVC)
    target_compile_options(${target_name} PRIVATE "$<$<CONFIG:Release>:/GL>")
    if(GRAPHION_PGO_MODE STREQUAL "GENERATE")
      target_link_options(${target_name} PRIVATE
        "$<$<CONFIG:Release>:/LTCG>"
        "$<$<CONFIG:Release>:/GENPROFILE>"
        "$<$<CONFIG:Release>:/PGD:${graphion_pgo_profile_dir_cmake}/${target_name}.pgd>"
      )
    elseif(GRAPHION_PGO_MODE STREQUAL "USE")
      target_link_options(${target_name} PRIVATE
        "$<$<CONFIG:Release>:/LTCG>"
        "$<$<CONFIG:Release>:/USEPROFILE>"
        "$<$<CONFIG:Release>:/PGD:${graphion_pgo_profile_dir_cmake}/${target_name}.pgd>"
      )
    endif()
  elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    if(GRAPHION_PGO_MODE STREQUAL "GENERATE")
      target_compile_options(${target_name} PRIVATE
        "$<$<CONFIG:Release>:-fprofile-generate=${graphion_pgo_profile_dir_cmake}>"
        "$<$<CONFIG:Release>:-fprofile-update=prefer-atomic>"
      )
      target_link_options(${target_name} PRIVATE
        "$<$<CONFIG:Release>:-fprofile-generate=${graphion_pgo_profile_dir_cmake}>"
      )
    elseif(GRAPHION_PGO_MODE STREQUAL "USE")
      target_compile_options(${target_name} PRIVATE
        "$<$<CONFIG:Release>:-fprofile-use=${graphion_pgo_profile_dir_cmake}>"
        "$<$<CONFIG:Release>:-fprofile-correction>"
      )
      target_link_options(${target_name} PRIVATE
        "$<$<CONFIG:Release>:-fprofile-use=${graphion_pgo_profile_dir_cmake}>"
      )
    endif()
  elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    if(GRAPHION_PGO_MODE STREQUAL "GENERATE")
      target_compile_options(${target_name} PRIVATE
        "$<$<CONFIG:Release>:-fprofile-instr-generate>"
      )
      target_link_options(${target_name} PRIVATE
        "$<$<CONFIG:Release>:-fprofile-instr-generate>"
      )
    elseif(GRAPHION_PGO_MODE STREQUAL "USE")
      target_compile_options(${target_name} PRIVATE
        "$<$<CONFIG:Release>:-fprofile-instr-use=${graphion_pgo_profdata_cmake}>"
      )
      target_link_options(${target_name} PRIVATE
        "$<$<CONFIG:Release>:-fprofile-instr-use=${graphion_pgo_profdata_cmake}>"
      )
    endif()
  else()
    message(FATAL_ERROR "GRAPHION_PGO_MODE requires MSVC, GCC, or Clang.")
  endif()
endfunction()

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
    if(NOT WIN32)
      add_compile_options("$<$<NOT:$<CONFIG:Debug>>:-fPIE>")
      add_link_options("$<$<NOT:$<CONFIG:Debug>>:-Wl,-z,relro,-z,now>")
      add_link_options("$<$<NOT:$<CONFIG:Debug>>:-pie>")
    endif()
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
  src/parser/frontend.c
  src/compiler/ir.c
  src/graph/csr_graph.c
  src/graph/hypergraph.c
)
graphion_enable_pgo(graphion_vm)

if(GRAPHION_VM_DISPATCH STREQUAL "switch")
  target_compile_definitions(graphion_vm PRIVATE GRAPHION_VM_DISPATCH_SWITCH=1)
elseif(GRAPHION_VM_DISPATCH STREQUAL "jumptable")
  target_compile_definitions(graphion_vm PRIVATE GRAPHION_VM_DISPATCH_JUMPTABLE=1)
elseif(GRAPHION_VM_DISPATCH STREQUAL "computed-goto")
  if(MSVC)
    message(FATAL_ERROR "GRAPHION_VM_DISPATCH=computed-goto is not supported with MSVC.")
  endif()
  target_compile_definitions(graphion_vm PRIVATE GRAPHION_VM_DISPATCH_COMPUTED_GOTO=1)
else()
  message(FATAL_ERROR "Invalid GRAPHION_VM_DISPATCH='${GRAPHION_VM_DISPATCH}'. Use switch, jumptable, or computed-goto.")
endif()

if(GRAPHION_ENABLE_IPO)
  check_ipo_supported(RESULT graphion_ipo_supported OUTPUT graphion_ipo_error)
  if(graphion_ipo_supported)
    set_property(TARGET graphion_vm PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET graphion_vm PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    set_property(TARGET graphion_vm PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
  else()
    message(WARNING "IPO requested but not supported: ${graphion_ipo_error}")
  endif()
endif()

if(GRAPHION_ENABLE_ASM)
  if(MSVC)
    message(FATAL_ERROR "GRAPHION_ENABLE_ASM currently requires GCC/Clang toolchains (GNU-style .s file).")
  elseif(WIN32)
    message(FATAL_ERROR "GRAPHION_ENABLE_ASM currently requires a SysV x86_64 ABI environment (Linux/macOS).")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64|x64")
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
graphion_enable_pgo(graphion)

if(GRAPHION_ENABLE_IPO AND graphion_ipo_supported)
  set_property(TARGET graphion PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
  set_property(TARGET graphion PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
  set_property(TARGET graphion PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
endif()

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
  graphion_enable_pgo(graphion_tests)
  if(GRAPHION_ENABLE_IPO AND graphion_ipo_supported)
    set_property(TARGET graphion_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET graphion_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    set_property(TARGET graphion_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
  endif()
  add_test(NAME graphion_tests COMMAND graphion_tests)
endif()

if(GRAPHION_ENABLE_BENCHMARKS)
  add_executable(graphion_bench
    benchmarks/bench_vm.c
  )
  target_link_libraries(graphion_bench PRIVATE graphion_vm)
  graphion_enable_pgo(graphion_bench)
  if(GRAPHION_ENABLE_IPO AND graphion_ipo_supported)
    set_property(TARGET graphion_bench PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET graphion_bench PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    set_property(TARGET graphion_bench PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
  endif()

  add_executable(graphion_bench_bfs
    benchmarks/bench_bfs.c
  )
  target_link_libraries(graphion_bench_bfs PRIVATE graphion_vm)
  graphion_enable_pgo(graphion_bench_bfs)
  if(GRAPHION_ENABLE_IPO AND graphion_ipo_supported)
    set_property(TARGET graphion_bench_bfs PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET graphion_bench_bfs PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    set_property(TARGET graphion_bench_bfs PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
  endif()

  add_executable(graphion_bench_hypergraph
    benchmarks/bench_hypergraph.c
  )
  target_link_libraries(graphion_bench_hypergraph PRIVATE graphion_vm)
  graphion_enable_pgo(graphion_bench_hypergraph)
  if(GRAPHION_ENABLE_IPO AND graphion_ipo_supported)
    set_property(TARGET graphion_bench_hypergraph PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET graphion_bench_hypergraph PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    set_property(TARGET graphion_bench_hypergraph PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
  endif()

  add_executable(graphion_bench_hypergraph_incident_sum
    benchmarks/bench_hypergraph_incident_sum.c
  )
  target_link_libraries(graphion_bench_hypergraph_incident_sum PRIVATE graphion_vm)
  graphion_enable_pgo(graphion_bench_hypergraph_incident_sum)
  if(GRAPHION_ENABLE_IPO AND graphion_ipo_supported)
    set_property(TARGET graphion_bench_hypergraph_incident_sum PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET graphion_bench_hypergraph_incident_sum PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    set_property(TARGET graphion_bench_hypergraph_incident_sum PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
  endif()

  add_executable(graphion_bench_hypergraph_hyperedge_node_sum
    benchmarks/bench_hypergraph_hyperedge_node_sum.c
  )
  target_link_libraries(graphion_bench_hypergraph_hyperedge_node_sum PRIVATE graphion_vm)
  graphion_enable_pgo(graphion_bench_hypergraph_hyperedge_node_sum)
  if(GRAPHION_ENABLE_IPO AND graphion_ipo_supported)
    set_property(TARGET graphion_bench_hypergraph_hyperedge_node_sum PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET graphion_bench_hypergraph_hyperedge_node_sum PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    set_property(TARGET graphion_bench_hypergraph_hyperedge_node_sum PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
  endif()

  add_executable(graphion_bench_vm_graph
    benchmarks/bench_vm_graph.c
  )
  target_link_libraries(graphion_bench_vm_graph PRIVATE graphion_vm)
  graphion_enable_pgo(graphion_bench_vm_graph)
  if(GRAPHION_ENABLE_IPO AND graphion_ipo_supported)
    set_property(TARGET graphion_bench_vm_graph PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET graphion_bench_vm_graph PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    set_property(TARGET graphion_bench_vm_graph PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
  endif()
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
  graphion_enable_pgo(fuzz_vm)

  add_executable(fuzz_parser
    fuzz/fuzz_parser.c
  )
  target_compile_options(fuzz_parser PRIVATE -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer)
  target_link_options(fuzz_parser PRIVATE -fsanitize=fuzzer,address,undefined)
  target_link_libraries(fuzz_parser PRIVATE graphion_vm)
  graphion_enable_pgo(fuzz_parser)
endif()
