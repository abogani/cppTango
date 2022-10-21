# Add the default cflags and include directories to ${target}
function(set_cflags_and_include target)
  target_link_libraries(${target}
    PUBLIC
        ${CMAKE_DL_LIBS}
        omniORB4::omniORB4
        omniORB4::COS4
        omniORB4::Dynamic4
        cppzmq::cppzmq
  )

  target_include_directories(${target} SYSTEM PUBLIC
    ${cppzmq_INCLUDE_DIR}
    ${omniORB4_INCLUDE_DIR}
  )

  if(TANGO_USE_JPEG)
    target_link_libraries(${target} PUBLIC jpeg)
    target_include_directories(${target} PRIVATE ${jpeg_INCLUDE_DIR})
  endif()
endfunction()
