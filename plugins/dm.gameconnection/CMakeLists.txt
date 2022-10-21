add_library(dm_gameconnection MODULE
            clsocket/ActiveSocket.cpp
            clsocket/PassiveSocket.cpp
            clsocket/SimpleSocket.cpp
            GameConnectionPanel.cpp
            DiffDoom3MapWriter.cpp
            AutomationEngine.cpp
            GameConnection.cpp
            MapObserver.cpp
            MessageTcp.cpp)
target_compile_options(dm_gameconnection PUBLIC ${SIGC_CFLAGS})
target_link_libraries(dm_gameconnection PUBLIC wxutil)

file(GLOB PNG_FILES "*.png")
install(FILES ${PNG_FILES} DESTINATION ${PKGDATADIR}/bitmaps)
