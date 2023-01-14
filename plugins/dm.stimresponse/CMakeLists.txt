add_library(dm_stimresponse MODULE
            ClassEditor.cpp
            CustomStimEditor.cpp
            EffectArgumentItem.cpp
            EffectEditor.cpp
            plugin.cpp
            ResponseEditor.cpp
            ResponseEffect.cpp
            ResponseEffectTypes.cpp
            SREntity.cpp
            SRPropertyLoader.cpp
            SRPropertyRemover.cpp
            SRPropertySaver.cpp
            StimEditor.cpp
            StimResponse.cpp
            StimResponseEditor.cpp
            StimTypes.cpp)
target_link_libraries(dm_stimresponse PUBLIC wxutil)

# Enable precompiled header for dm_stimresponse
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
target_precompile_headers(dm_stimresponse PRIVATE "precompiled.h")
endif()