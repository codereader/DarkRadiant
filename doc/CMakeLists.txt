add_custom_command(OUTPUT manual.html
                   COMMAND ${ASCIIDOCTOR} ARGS -a stylesheet=manual.css manual.adoc
                   DEPENDS manual.adoc)
add_custom_target(doc ALL DEPENDS manual.html)
install(FILES manual.html
        DESTINATION ${CMAKE_INSTALL_DOCDIR})
