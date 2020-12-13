add_executable(drtest
               Camera.cpp
               ColourSchemes.cpp
               CSG.cpp
               Face.cpp
               FacePlane.cpp
               FileTypes.cpp
               HeadlessOpenGLContext.cpp
               MapExport.cpp
               MapSavingLoading.cpp
               Materials.cpp
               math/Matrix4.cpp
               math/Plane3.cpp
               math/Quaternion.cpp
               math/Vector3.cpp
               MessageBus.cpp
               ModelExport.cpp
               ModelScale.cpp
               Models.cpp
               PatchIterators.cpp
               PatchWelding.cpp
               SelectionAlgorithm.cpp
               Selection.cpp
               VFS.cpp
               WorldspawnColour.cpp)
target_compile_options(drtest PRIVATE ${SIGC_CFLAGS})
target_link_libraries(drtest PRIVATE
                      math xmlutil module
                      ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES}
                      ${SIGC_LIBRARIES} ${GLEW_LIBRARIES} ${X11_LIBRARIES})
