if(NOT DEFINED GRAPHION_CLI OR NOT DEFINED GRAPHION_CLI_TEST_DIR)
  message(FATAL_ERROR "GRAPHION_CLI and GRAPHION_CLI_TEST_DIR are required")
endif()

file(REMOVE_RECURSE "${GRAPHION_CLI_TEST_DIR}")
file(MAKE_DIRECTORY "${GRAPHION_CLI_TEST_DIR}")

set(warning_path "${GRAPHION_CLI_TEST_DIR}/warning_emission.gion")
file(WRITE "${warning_path}"
  "match \"a\":\n"
  "    1:\n"
  "        print(\"bad\")\n"
  "    default:\n"
  "        print(\"ran\")\n"
)

execute_process(
  COMMAND "${GRAPHION_CLI}" -d "${warning_path}"
  RESULT_VARIABLE warning_result
  OUTPUT_VARIABLE warning_stdout
  ERROR_VARIABLE warning_stderr
)
if(NOT warning_result EQUAL 0)
  message(FATAL_ERROR "graphion -d warning program exited with ${warning_result}: ${warning_stderr}")
endif()
if(NOT warning_stdout STREQUAL "ran\n")
  message(FATAL_ERROR "graphion -d did not execute warning program as expected: '${warning_stdout}'")
endif()
if(NOT warning_stderr STREQUAL "warning:2:5: match case can never match a string value\n")
  message(FATAL_ERROR "graphion -d emitted unexpected warning output: '${warning_stderr}'")
endif()

execute_process(
  COMMAND "${GRAPHION_CLI}" "${warning_path}"
  RESULT_VARIABLE plain_result
  OUTPUT_VARIABLE plain_stdout
  ERROR_VARIABLE plain_stderr
)
if(NOT plain_result EQUAL 0)
  message(FATAL_ERROR "graphion warning program exited with ${plain_result}: ${plain_stderr}")
endif()
if(NOT plain_stdout STREQUAL "ran\n")
  message(FATAL_ERROR "graphion did not execute warning program without -d as expected: '${plain_stdout}'")
endif()
if(NOT plain_stderr STREQUAL "")
  message(FATAL_ERROR "graphion emitted a debug warning without -d: '${plain_stderr}'")
endif()

set(capacity_path "${GRAPHION_CLI_TEST_DIR}/warning_capacity.gion")
set(capacity_source "match \"a\":\n")
foreach(case_id RANGE 0 32)
  string(APPEND capacity_source "    ${case_id}:\n        print(1)\n")
endforeach()
string(APPEND capacity_source "print(\"should-not-run\")\n")
file(WRITE "${capacity_path}" "${capacity_source}")

execute_process(
  COMMAND "${GRAPHION_CLI}" -d "${capacity_path}"
  RESULT_VARIABLE capacity_result
  OUTPUT_VARIABLE capacity_stdout
  ERROR_VARIABLE capacity_stderr
)
if(NOT capacity_result EQUAL 3)
  message(FATAL_ERROR "graphion -d capacity program exited with ${capacity_result}: ${capacity_stderr}")
endif()
if(NOT capacity_stdout STREQUAL "")
  message(FATAL_ERROR "graphion executed a program after warning collection failed: '${capacity_stdout}'")
endif()
if(NOT capacity_stderr STREQUAL "error:66:5: warning capacity exceeded\n")
  message(FATAL_ERROR "graphion -d emitted unexpected capacity error: '${capacity_stderr}'")
endif()

file(REMOVE_RECURSE "${GRAPHION_CLI_TEST_DIR}")
