add_library(module
            ApplicationContextBase.cpp
            CoreModule.cpp
            DynamicLibrary.cpp
            StaticModule.cpp)
target_compile_options(module PUBLIC ${SIGC_CFLAGS})
