add_library(xmlutil Document.cpp Node.cpp)

# link with system puixml if we have it.
if (TARGET pugixml::pugixml)
  target_link_libraries(xmlutil PUBLIC pugixml::pugixml)
endif()
