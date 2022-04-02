add_library(xmlutil
            Document.cpp Node.cpp XmlModule.cpp)
target_compile_options(xmlutil PUBLIC ${XML_CFLAGS})
target_link_libraries(xmlutil PUBLIC ${XML_LIBRARIES})
