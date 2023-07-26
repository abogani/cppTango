include(FindDoxygen)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

if(DOXYGEN_FOUND AND DOXYGEN_DOT_FOUND)
  add_custom_target(doc doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../server
                    SOURCES Doxyfile)
else()
  message(WARNING "Could not find doxygen/dot, therefore the documentation can not be built.")
endif()
