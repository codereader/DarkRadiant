add_library(dm_difficulty MODULE
            ClassNameStore.cpp
            DifficultyDialog.cpp
            DifficultyEditor.cpp
            DifficultyEntity.cpp
            DifficultySettings.cpp
            DifficultySettingsManager.cpp
            plugin.cpp
            Setting.cpp)
target_compile_options(dm_difficulty PUBLIC ${SIGC_CFLAGS})
target_link_libraries(dm_difficulty PUBLIC wxutil)
