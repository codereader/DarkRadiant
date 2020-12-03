add_library(xmlutil
            Document.cpp Node.cpp)
target_compile_options(xmlutil PUBLIC ${XML_CFLAGS})
