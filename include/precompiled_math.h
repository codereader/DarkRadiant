/** 
 * greebo: Precompiled header module. This is included by the respective precompiled.h
 * files throughout the project. Many of those include boost headers into the
 * pre-compilation, and they do so by #include'ing this file.
 */
#pragma once

// Include important math headers
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4.h"
#include "math/AABB.h"
#include "math/Quaternion.h"
#include "math/Frustum.h"
#include "math/Plane3.h"
