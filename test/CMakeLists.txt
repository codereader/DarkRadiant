include(GoogleTest)

add_executable(drtest
               Basic.cpp
               Brush.cpp
               Camera.cpp
               ColourSchemes.cpp
               ContinuousBuffer.cpp
               CSG.cpp
               Entity.cpp
               Favourites.cpp
               FileTypes.cpp
               GeometryStore.cpp
               Grid.cpp
               HeadlessOpenGLContext.cpp
               ImageLoading.cpp
               LayerManipulation.cpp
               MapExport.cpp
               MapMerging.cpp
               MapSavingLoading.cpp
               MaterialExport.cpp
               Materials.cpp
               math/Matrix3.cpp
               math/Matrix4.cpp
               math/Plane3.cpp
               math/Quaternion.cpp
               math/Vector.cpp
               MessageBus.cpp
               ModelExport.cpp
               ModelScale.cpp
               Models.cpp
               Particles.cpp
               PatchIterators.cpp
               PatchWelding.cpp
               PointTrace.cpp
               Prefabs.cpp
               Renderer.cpp
               SceneNode.cpp
               SelectionAlgorithm.cpp
               Selection.cpp
               TextureManipulation.cpp
               TextureTool.cpp
               Transformation.cpp
               UndoRedo.cpp
               VFS.cpp
               WorldspawnColour.cpp)

find_package(Threads REQUIRED)

target_compile_options(drtest PUBLIC ${SIGC_CFLAGS})

# Set up the paths such that the drtest executable can find the test resources
# and the core binary in the build workspace (in install/ and test/resources/)
get_filename_component(TEST_BASE_PATH "./" ABSOLUTE)
add_compile_definitions(TEST_BASE_PATH="${TEST_BASE_PATH}")

target_link_libraries(drtest PUBLIC
                      math xmlutil scenegraph module
                      ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES}
                      ${SIGC_LIBRARIES} ${GLEW_LIBRARIES} ${X11_LIBRARIES}
                      PRIVATE Threads::Threads)
install(TARGETS drtest)

gtest_discover_tests(drtest)