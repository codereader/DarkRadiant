add_executable(drtest
               Camera.cpp
               ColourSchemes.cpp
               CSG.cpp
               Face.cpp
               FacePlane.cpp
               Favourites.cpp
               FileTypes.cpp
               HeadlessOpenGLContext.cpp
               ImageLoading.cpp
               LayerManipulation.cpp
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
               Prefabs.cpp
               SelectionAlgorithm.cpp
               Selection.cpp
               VFS.cpp
               WorldspawnColour.cpp)
               
find_package(Threads REQUIRED)
 
target_compile_options(drtest PUBLIC ${SIGC_CFLAGS})

get_filename_component(TESTRESOURCEDIR "./resources" ABSOLUTE)
add_compile_definitions(TESTRESOURCEDIR="${TESTRESOURCEDIR}")

target_link_libraries(drtest PUBLIC
                      math xmlutil scenegraph module
                      ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES}
                      ${SIGC_LIBRARIES} ${GLEW_LIBRARIES} ${X11_LIBRARIES}
                      PRIVATE Threads::Threads)
install(TARGETS drtest)
