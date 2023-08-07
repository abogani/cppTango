set(HEADER_FILES
                Appender.h
                AppenderAttachable.h
                LayoutAppender.h
                FileAppender.h
                RollingFileAppender.h
                OstreamAppender.h
                Layout.h
                PatternLayout.h
                XmlLayout.h
                Logger.h
                LogSeparator.h
                LoggerStream.h
                LogStreambuf.h
                LoggingEvent.h
                Level.h
                Filter.h
                Portability.h)

install(FILES ${HEADER_FILES}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tango/common/log4tango)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/config.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tango/common/log4tango)
