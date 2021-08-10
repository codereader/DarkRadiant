add_library(math
            AABB.cpp Frustum.cpp Matrix4.cpp Plane3.cpp SHA256.cpp)
target_compile_options(math PUBLIC ${EIGEN_CFLAGS})