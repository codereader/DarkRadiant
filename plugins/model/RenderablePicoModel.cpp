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
		// Test model.
		glBegin(GL_QUADS);
			// Top
			glColor3f(1, 0, 0); glNormal3f(0, 1, 0);
			glVertex3f(1, 1, 1);
			glVertex3f(1, 1, -1);
			glVertex3f(-1, 1, -1);
			glVertex3f(-1, 1, 1);
			// Front
			glColor3f(1, 1, 0); glNormal3f(0, 0, 1);
			glVertex3f(1, 1, 1);
			glVertex3f(-1, 1, 1);
			glVertex3f(-1, -1, 1);
			glVertex3f(1, -1, 1);
			// Right
			glColor3f(0, 1, 0); glNormal3f(1, 0, 0);
			glVertex3f(1, 1, 1);
			glVertex3f(1, -1, 1);
			glVertex3f(1, -1, -1);
			glVertex3f(1, 1, -1);
			// Left
			glColor3f(0, 1, 1); glNormal3f(-1, 0, 0);
			glVertex3f(-1, 1, 1);
			glVertex3f(-1, 1, -1);
			glVertex3f(-1, -1, -1);
			glVertex3f(-1, -1, 1);
			// Bottom
			glColor3f(0, 0, 1); glNormal3f(0, -1, 0);
			glVertex3f(1, -1, 1);
			glVertex3f(-1, -1, 1);
			glVertex3f(-1, -1, -1);
			glVertex3f(1, -1, -1);
			// Back
			glColor3f(1, 0, 1); glNormal3f(0, 0, -1);
			glVertex3f(1, 1, -1);
			glVertex3f(1, -1, -1);
			glVertex3f(-1, -1, -1);
			glVertex3f(-1, 1, -1);
		glEnd();
}
	
}
