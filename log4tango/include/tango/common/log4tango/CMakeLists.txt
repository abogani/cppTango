set(HEADER_FILES
                Appender.hh
                AppenderAttachable.hh
                LayoutAppender.hh
                FileAppender.hh
                RollingFileAppender.hh
                OstreamAppender.hh
                Layout.hh
                PatternLayout.hh
                XmlLayout.hh
                Logger.hh
                LogSeparator.hh
                LoggerStream.hh
                LogStreambuf.hh
                LoggingEvent.hh
                Level.hh
                Filter.hh
                Portability.hh)

install(FILES ${HEADER_FILES}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tango/common/log4tango)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/config.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tango/common/log4tango)
