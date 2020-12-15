add_library(dm_gui MODULE
            gui/Gui.cpp
            gui/GuiExpression.cpp
            gui/GuiManager.cpp
            gui/GuiScript.cpp
            gui/GuiWindowDef.cpp
            gui/RenderableCharacterBatch.cpp
            gui/RenderableText.cpp
            GuiSelector.cpp
            gui/Variable.cpp
            plugin.cpp
            ReadableEditorDialog.cpp
            ReadableGuiView.cpp
            XData.cpp
            XDataLoader.cpp
            XDataSelector.cpp
            XdFileChooserDialog.cpp)
target_compile_options(dm_gui PUBLIC ${SIGC_CFLAGS})
target_link_libraries(dm_gui PUBLIC wxutil)
