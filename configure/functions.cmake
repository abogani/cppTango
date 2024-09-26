# Add the default cflags and include directories to ${target}
function(set_cflags_and_include target)

  target_link_libraries(${target} PUBLIC ${CMAKE_DL_LIBS})

  set(libraries
      omniORB4::omniORB4
      omniORB4::COS4
      omniORB4::Dynamic4
      cppzmq::cppzmq)

  foreach(lib IN LISTS libraries)
      if (WIN32 AND NOT BUILD_SHARED_LIBS AND TARGET ${lib}-static)
          target_link_libraries(${target} PUBLIC ${lib}-static)
      else()
          target_link_libraries(${target} PUBLIC ${lib})
      endif()
  endforeach()

  target_include_directories(${target} PRIVATE
    ${TANGO_SOURCE_DIR}/src/include
    ${TANGO_SOURCE_DIR}/log4tango/include
    ${TANGO_BINARY_DIR}/src/include
    ${TANGO_BINARY_DIR}/log4tango/include
  )

  if(TANGO_USE_JPEG)
    if (WIN32 AND NOT BUILD_SHARED_LIBS AND TARGET JPEG::JPEG-static)
      target_link_libraries(${target} PRIVATE JPEG::JPEG-static)
    else()
      target_link_libraries(${target} PRIVATE JPEG::JPEG)
    endif()
  endif()

  if (TANGO_USE_TELEMETRY)
      target_link_libraries(${target} PRIVATE
                            opentelemetry-cpp::trace
                            opentelemetry-cpp::sdk
                            opentelemetry-cpp::api
                            opentelemetry-cpp::ostream_log_record_exporter
                            opentelemetry-cpp::ostream_span_exporter
                            opentelemetry-cpp::logs
                            ZLIB::ZLIB
                           )
      if (TANGO_TELEMETRY_USE_HTTP)
        target_link_libraries(${target} PRIVATE
                              opentelemetry-cpp::otlp_http_exporter
                              opentelemetry-cpp::otlp_http_log_record_exporter
                             )
      endif()
      if (TANGO_TELEMETRY_USE_GRPC)
        target_link_libraries(${target} PRIVATE
                              opentelemetry-cpp::otlp_grpc_exporter
                              opentelemetry-cpp::otlp_grpc_log_record_exporter
                             )
        endif()
  endif()

endfunction()
