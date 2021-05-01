add_library(module STATIC
            ApplicationContextBase.cpp
            CoreModule.cpp
            DynamicLibrary.cpp
            StaticModule.cpp)
target_compile_options(module PUBLIC -fPIC ${SIGC_CFLAGS} ${EIGEN_CFLAGS})
target_link_libraries(module PUBLIC dl)
