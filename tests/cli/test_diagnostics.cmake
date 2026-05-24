if(NOT DEFINED GRAPHION_CLI OR NOT DEFINED GRAPHION_CLI_TEST_DIR)
  message(FATAL_ERROR "GRAPHION_CLI and GRAPHION_CLI_TEST_DIR are required")
endif()

file(REMOVE_RECURSE "${GRAPHION_CLI_TEST_DIR}")
file(MAKE_DIRECTORY "${GRAPHION_CLI_TEST_DIR}")

function(graphion_expect_cli_error case_name source_text expected_stderr)
  set(case_path "${GRAPHION_CLI_TEST_DIR}/${case_name}.gion")
  file(WRITE "${case_path}" "${source_text}")

  execute_process(
    COMMAND "${GRAPHION_CLI}" "${case_path}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
  )
  if(NOT result EQUAL 3)
    message(FATAL_ERROR "graphion ${case_name} exited with ${result}: '${stderr}'")
  endif()
  if(NOT stdout STREQUAL "")
    message(FATAL_ERROR "graphion ${case_name} unexpectedly wrote stdout: '${stdout}'")
  endif()
  if(NOT stderr STREQUAL "${expected_stderr}")
    message(FATAL_ERROR "graphion ${case_name} emitted unexpected stderr: '${stderr}'")
  endif()
endfunction()

graphion_expect_cli_error(
  "parse_error"
  "count =\n"
  "error:1:7: expected expression after '='\n"
)
graphion_expect_cli_error(
  "name_error"
  "print(missing)\n"
  "error:1:7: unknown operand 'missing'\n"
)
graphion_expect_cli_error(
  "runtime_error"
  "value = 1 / 0\n"
  "error:1:1: division by zero\n"
)

set(extension_path "${GRAPHION_CLI_TEST_DIR}/not_gion.txt")
file(WRITE "${extension_path}" "print(1)\n")
execute_process(
  COMMAND "${GRAPHION_CLI}" "${extension_path}"
  RESULT_VARIABLE extension_result
  OUTPUT_VARIABLE extension_stdout
  ERROR_VARIABLE extension_stderr
)
if(NOT extension_result EQUAL 2)
  message(FATAL_ERROR "graphion invalid extension exited with ${extension_result}: '${extension_stderr}'")
endif()
if(NOT extension_stdout STREQUAL "")
  message(FATAL_ERROR "graphion invalid extension unexpectedly wrote stdout: '${extension_stdout}'")
endif()
if(NOT extension_stderr STREQUAL "error: source file must use the .gion extension\n")
  message(FATAL_ERROR "graphion invalid extension emitted unexpected stderr: '${extension_stderr}'")
endif()

file(REMOVE_RECURSE "${GRAPHION_CLI_TEST_DIR}")
