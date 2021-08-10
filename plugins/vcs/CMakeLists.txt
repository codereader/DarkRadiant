add_library(vcs MODULE
            GitModule.cpp
            Index.cpp
            Repository.cpp
            ui/VcsStatus.cpp)
target_compile_options(vcs PUBLIC ${SIGC_CFLAGS})
target_link_libraries(vcs PUBLIC wxutil ${LIBGIT_LIBRARIES})
