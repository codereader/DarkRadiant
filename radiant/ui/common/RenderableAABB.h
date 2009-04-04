#ifndef RENDERABLEAABB_H_
#define RENDERABLEAABB_H_

#include "irender.h"
#include "math/aabb.h"

namespace ui
{

/** Adapter class that allows the rendering of an AABB as a wireframe cuboid. The
 * class implements the OpenGLRenderable interface and accepts an AABB as a 
 * construction parameter.
 */

class RenderableAABB
: public OpenGLRenderable
{
	// The AABB to render
	AABB _aabb;
	
public:

	/** Construct a RenderableAABB to render the provided AABB.
	 */
	RenderableAABB(const AABB& aabb)
	: _aabb(aabb) {}
	
	/** Render function from OpenGLRenderable interface.
	 */
	void render(const RenderInfo& info) const;
};

}

#endif /*RENDERABLEAABB_H_*/
