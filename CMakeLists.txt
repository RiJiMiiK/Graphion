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
  src/vm/internal/core/value.c
  src/vm/internal/opcodes/op_scalar.c
  src/vm/internal/opcodes/op_state.c
  src/vm/internal/opcodes/op_io.c
  src/vm/internal/opcodes/op_frontier.c
  src/vm/internal/opcodes/op_graph.c
  src/vm/internal/opcodes/op_hypergraph.c
  src/vm/internal/core/sum.c
  src/vm/internal/core/fastpath_plan.c
  src/vm/internal/core/fastpath_exec.c
  src/vm/internal/core/dispatch.c
  src/runtime/arena.c
  src/runtime/entry.c
  src/runtime/interpreter.c
  src/runtime/interpreter/base.c
  src/runtime/interpreter/program.c
  src/runtime/interpreter/operands.c
  src/runtime/interpreter/expr.c
  src/runtime/interpreter/stmt.c
  src/runtime/interpreter/source.c
  src/runtime/interpreter/exec.c
  src/parser/bytecode.c
  src/parser/frontend.c
  src/compiler/ir.c
  src/graph/csr_graph.c
  src/graph/hypergraph.c
)

if(NOT MSVC)
  target_link_libraries(graphion_vm PUBLIC m)
endif()


if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  if(MSVC)
    set_source_files_properties(src/vm/vm.c PROPERTIES COMPILE_OPTIONS "/arch:AVX2")
  elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(src/vm/vm.c PROPERTIES COMPILE_OPTIONS "-mavx2")
  endif()
endif()

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

if(GRAPHION_ENABLE_IPO AND graphion_ipo_supported)
  set_property(TARGET graphion PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
  set_property(TARGET graphion PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
  set_property(TARGET graphion PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
endif()

if(GRAPHION_ENABLE_TESTS)
  enable_testing()

  add_executable(graphion_tests
    tests/runner/test_main.c
    tests/vm/test_vm_helpers.c
    tests/vm/test_vm_core.c
    tests/vm/test_vm_scalars.c
    tests/vm/test_vm_logic_bits.c
    tests/vm/test_vm_graph_ops.c
    tests/core/test_arena.c
    tests/gion/test_parser_frontend.c
    tests/gion/test_gion_scalars.c
    tests/gion/test_gion_bits.c
    tests/gion/test_gion_errors_misc.c
    tests/gion/test_gion_control_flow.c
    tests/gion/test_gion_operators.c
    tests/core/test_isa.c
    tests/graph/test_graph.c
    tests/graph/test_hypergraph.c
  )
  target_include_directories(graphion_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/core
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/gion
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/vm
  )
  target_link_libraries(graphion_tests PRIVATE graphion_vm)
  if(GRAPHION_ENABLE_IPO AND graphion_ipo_supported)
    set_property(TARGET graphion_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET graphion_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    set_property(TARGET graphion_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
  endif()
  add_test(NAME graphion_tests COMMAND graphion_tests)
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

