add_library(scenegraph
            ChildPrimitives.cpp
            InstanceWalkers.cpp
            LayerUsageBreakdown.cpp
            ModelFinder.cpp
            Node.cpp
            merge/MergeOperation.cpp
            merge/MergeOperationBase.cpp
            merge/MergeActionNode.cpp
            merge/GraphComparer.cpp
            merge/ThreeWayMergeOperation.cpp
            SelectableNode.cpp
            SelectionIndex.cpp
            TraversableNodeSet.cpp
            Traverse.cpp)
target_compile_options(scenegraph PUBLIC ${SIGC_CFLAGS})
target_link_libraries(scenegraph PUBLIC ${SIGC_LIBRARIES} math)
