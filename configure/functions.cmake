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
      target_link_libraries(${target} PRIVATE JPEG::JPEG)
  endif()

  if (TANGO_USE_TELEMETRY)
      target_link_libraries(${target} PRIVATE
                            opentelemetry-cpp::trace
			    opentelemetry-cpp::sdk
			    opentelemetry-cpp::api
			    opentelemetry-cpp::ostream_log_record_exporter
			    opentelemetry-cpp::ostream_span_exporter
			    opentelemetry-cpp::logs
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
