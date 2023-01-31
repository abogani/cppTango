# Add the default cflags and include directories to ${target}
function(set_cflags_and_include target)
  target_compile_options(${target} PUBLIC ${ZMQ_PKG_CFLAGS_OTHER}
                                          ${JPEG_PKG_CFLAGS_OTHER}
                                          ${OMNIORB_PKG_CFLAGS_OTHER}
                                          ${OMNICOS_PKG_CFLAGS_OTHER}
                                          ${OMNIDYN_PKG_CFLAGS_OTHER})

  target_include_directories(${target} SYSTEM PUBLIC ${ZMQ_PKG_INCLUDE_DIRS}
                                                     ${JPEG_PKG_INCLUDE_DIRS}
                                                     ${OMNIORB_PKG_INCLUDE_DIRS}
                                                     ${OMNIDYN_PKG_INCLUDE_DIRS})
endfunction()
