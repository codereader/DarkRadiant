add_library(sound MODULE
            sound.cpp
            SoundManager.cpp
            SoundPlayer.cpp
            SoundShader.cpp)
target_compile_options(sound PUBLIC ${SIGC_CFLAGS})
target_link_libraries(sound PUBLIC wxutil ${AL_LIBRARIES} ${VORBIS_LIBRARIES})
