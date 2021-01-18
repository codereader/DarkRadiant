add_library(dm_conversation MODULE
            ActorNodeFinder.h
            CMakeLists.txt
            CommandArgumentItem.cpp
            CommandArgumentItem.h
            CommandEditor.cpp
            CommandEditor.h
            ConversationCommand.cpp
            ConversationCommand.h
            ConversationCommandInfo.cpp
            ConversationCommandInfo.h
            ConversationCommandLibrary.cpp
            ConversationCommandLibrary.h
            ConversationDialog.cpp
            ConversationDialog.h
            ConversationEditor.cpp
            ConversationEditor.h
            ConversationEntity.cpp
            ConversationEntityFinder.h
            ConversationEntity.h
            Conversation.h
            ConversationKeyExtractor.cpp
            ConversationKeyExtractor.h
            plugin.cpp)
target_compile_options(dm_conversation PUBLIC ${SIGC_CFLAGS})
target_link_libraries(dm_conversation PUBLIC wxutil)
