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
target_compile_options(dm_stimresponse PUBLIC ${SIGC_CFLAGS})
target_link_libraries(dm_stimresponse PUBLIC wxutil)
