#ifndef RENDERABLEPICOMODEL_H_
#define RENDERABLEPICOMODEL_H_

#include "irender.h"
#include "picomodel.h"

#include <GL/glew.h>

namespace model
{

/* Renderable class containing a model loaded via the picomodel library. A
 * RenderablePicoModel is made up of one or more RenderablePicoSurface objects,
 * each of which contains a number of polygons with the same texture. Rendering
 * a RenderablePicoModel involves rendering all of its surfaces, each of which
 * binds its texture(s) and submits its geometry via OpenGL calls.
 */

class RenderablePicoModel
: public OpenGLRenderable
{
public:

	/** Constructor. Accepts a picoModel_t struct containing the raw model data
	 * loaded from picomodel, and a string filename extension to allow the
	 * correct handling of material paths (which differs between ASE and LWO)
	 */
	
	RenderablePicoModel(const picoModel_t* mod, const std::string& fExt) {

	}
	
	/** Virtual render function from OpenGLRenderable.
	 */
	 
	void render(RenderStateFlags flags) const;
};

}

#endif /*RENDERABLEPICOMODEL_H_*/
