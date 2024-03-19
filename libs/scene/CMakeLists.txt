add_library(scene
            AttachmentData.cpp
            ChildPrimitives.cpp
            InstanceWalkers.cpp
            EntityKeyValue.cpp
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
            SpawnArgs.cpp
            TraversableNodeSet.cpp
            Traverse.cpp)
target_compile_options(scene PUBLIC ${SIGC_CFLAGS})
target_link_libraries(scene PUBLIC ${SIGC_LIBRARIES} math)
