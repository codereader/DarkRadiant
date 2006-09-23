#include "RenderablePicoModel.h"


namespace model {

// Constructor

RenderablePicoModel::RenderablePicoModel(picoModel_t* mod, const std::string& fExt) {
	
	// Get the number of surfaces to create
	int nSurf = PicoGetModelNumSurfaces(mod);
	
	// Create a RenderablePicoSurface for each surface in the structure
	for (int n = 0; n < nSurf; ++n) {
	
		// Retrieve the surface, discarding it if it is null or non-triangulated (?)
		picoSurface_t* surf = PicoGetModelSurface(mod, n);
		if (surf == 0 || PicoGetSurfaceType(surf) != PICO_TRIANGLES)
			continue;
			
		// Fix the normals of the surface (?)
		PicoFixSurfaceNormals(surf);
		
		// Create the RenderablePicoSurface object and add it to the vector
		boost::shared_ptr<RenderablePicoSurface> rSurf(new RenderablePicoSurface(surf, fExt));
		_surfVec.push_back(rSurf);
	}
	
}

// Virtual render function

void RenderablePicoModel::render(RenderStateFlags flags) const {
	
	glEnable(GL_VERTEX_ARRAY);
	glEnable(GL_NORMAL_ARRAY);
	glShadeModel(GL_SMOOTH);
	glColor3f(1.0, 1.0, 1.0);
	
	// Iterate over the surfaces, calling the render function on each one
	for (SurfaceList::const_iterator i = _surfVec.begin();
			 i != _surfVec.end();
			 ++i)
	{
		(*i)->render(flags);
	}
}
	
}
