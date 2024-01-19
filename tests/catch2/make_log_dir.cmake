execute_process(COMMAND ${CMAKE_COMMAND} -E rm -rf "${TANGO_CATCH2_LOG_DIR}")
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${TANGO_CATCH2_LOG_DIR}")
