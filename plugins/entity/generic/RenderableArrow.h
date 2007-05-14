#ifndef RENDERABLEARROW_H_
#define RENDERABLEARROW_H_

#include "irender.h"
#include "math/line.h"

#include "entitylib.h"

namespace entity {

class RenderableArrow : 
	public OpenGLRenderable
{
	const Ray& _ray;

public:
	RenderableArrow(const Ray& ray)	: 
		_ray(ray)
	{}

	void render(RenderStateFlags state) const {
		arrow_draw(_ray.origin, _ray.direction);
	}
};

} // namespace entity

#endif /*RENDERABLEARROW_H_*/
