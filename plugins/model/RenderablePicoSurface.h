#ifndef RENDERABLEPICOSURFACE_H_
#define RENDERABLEPICOSURFACE_H_

#include "picomodel.h"
#include "irender.h"

namespace model
{

/* Renderable class containing a series of polygons textured with the same
 * material. RenderablePicoSurface objects are composited into a RenderablePicoModel
 * object to create a renderable static mesh.
 */

class RenderablePicoSurface
: public OpenGLRenderable
{
public:

	/** Constructor. Accepts a picoSurface_t struct and the file extension to determine
	 * how to assign materials.
	 */
	 
	RenderablePicoSurface(picoSurface_t* surf, const std::string& fExt) {
		
	}
	
	/** Render function from OpenGLRenderable
	 */
	 
	void render(RenderStateFlags flags) const {}
	
};

}

#endif /*RENDERABLEPICOSURFACE_H_*/
