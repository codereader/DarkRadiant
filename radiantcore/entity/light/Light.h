#pragma once

#include "igl.h"
#include "irender.h"
#include "editable.h"
#include "render.h"
#include "irenderable.h"
#include "math/Frustum.h"
#include "transformlib.h"
#include "scene/TransformedCopy.h"

#include "../OriginKey.h"
#include "../RotationKey.h"
#include "../ColourKey.h"
#include "../ModelKey.h"
#include "../SpawnArgs.h"
#include "../KeyObserverDelegate.h"

#include "Renderables.h"
#include "LightShader.h"
#include "RenderableVertices.h"
#include "Doom3LightRadius.h"

namespace entity {

void light_vertices(const AABB& aabb_light, Vector3 points[6]);
void light_draw(const AABB& aabb_light, RenderStateFlags state);

inline void default_extents(Vector3& extents) {
	extents = Vector3(8,8,8);
}

} // namespace entity
