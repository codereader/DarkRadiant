add_library(dm_conversation MODULE
            CommandArgumentItem.cpp
            CommandEditor.cpp
            ConversationCommand.cpp
            ConversationCommandInfo.cpp
            ConversationCommandLibrary.cpp
            ConversationDialog.cpp
            ConversationEditor.cpp
            ConversationEntity.cpp
            ConversationKeyExtractor.cpp
            plugin.cpp)
target_compile_options(dm_conversation PUBLIC ${SIGC_CFLAGS})
target_link_libraries(dm_conversation PUBLIC wxutil)
