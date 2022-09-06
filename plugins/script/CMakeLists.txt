add_library(script MODULE
            interfaces/BrushInterface.cpp
            interfaces/CameraInterface.cpp
            interfaces/CommandSystemInterface.cpp
            interfaces/DeclarationManagerInterface.cpp
            interfaces/DialogInterface.cpp
            interfaces/EClassInterface.cpp
            interfaces/EntityInterface.cpp
            interfaces/FileSystemInterface.cpp
            interfaces/FxManagerInterface.cpp
            interfaces/GameInterface.cpp
            interfaces/GridInterface.cpp
            interfaces/LayerInterface.cpp
            interfaces/MapInterface.cpp
            interfaces/MathInterface.cpp
            interfaces/ModelInterface.cpp
            interfaces/PatchInterface.cpp
            interfaces/RadiantInterface.cpp
            interfaces/SceneGraphInterface.cpp
            interfaces/SelectionGroupInterface.cpp
            interfaces/SelectionInterface.cpp
            interfaces/SelectionSetInterface.cpp
            interfaces/ShaderSystemInterface.cpp
            interfaces/SkinInterface.cpp
            interfaces/SoundInterface.cpp
            PythonModule.cpp
            SceneNodeBuffer.cpp
            ScriptCommand.cpp
            ScriptingSystem.cpp
            ScriptModule.cpp)
set_target_properties(script PROPERTIES CXX_VISIBILITY_PRESET hidden)
target_include_directories(script SYSTEM PUBLIC
                           ${CMAKE_SOURCE_DIR}/libs/pybind
                           ${Python_INCLUDE_DIRS})
target_link_libraries(script PUBLIC
                      math
                      ${Python_LIBRARIES} ${SIGC_LIBRARIES})

# Enable precompiled header for script plugin
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
target_precompile_headers(script PRIVATE "precompiled.h")
endif()