execute_process(COMMAND ${CMAKE_COMMAND} -E rm -rf "${TANGO_BDD_LOG_DIR}")
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${TANGO_BDD_LOG_DIR}")
