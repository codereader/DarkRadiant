#include "RenderablePicoModel.h"

#include "texturelib.h"

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
		
		// Extend the model AABB to include the surface's AABB
		aabb_extend_by_aabb(_localAABB, rSurf->getAABB());
	}
	
}

// Virtual render function

void RenderablePicoModel::render(RenderStateFlags flags) const {
	
	glEnable(GL_VERTEX_ARRAY);
	glEnable(GL_NORMAL_ARRAY);
	glEnable(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	
	// Iterate over the surfaces, calling the render function on each one
	for (SurfaceList::const_iterator i = _surfVec.begin();
			 i != _surfVec.end();
			 ++i)
	{
		qtexture_t& tex = (*i)->getShader()->getTexture();
		glBindTexture(GL_TEXTURE_2D, tex.texture_number);
		(*i)->render(flags);
	}
}
	
// Apply the given skin to this model

void RenderablePicoModel::applySkin(const ModelSkin& skin) {
	// Apply the skin to each surface
	for (SurfaceList::iterator i = _surfVec.begin();
		 i != _surfVec.end();
		 ++i)
	{
		(*i)->applySkin(skin);
	}
}

// Update the list of active materials

void RenderablePicoModel::updateMaterialList() const {
	_materialList.clear();
	for (SurfaceList::const_iterator i = _surfVec.begin();
		 i != _surfVec.end();
		 ++i)
	{
		_materialList.push_back((*i)->getActiveMaterial());
	}
}

// Return the list of active skins for this model

const std::vector<std::string>& RenderablePicoModel::getActiveMaterials() const {
	// If the material list is empty, populate it
	if (_materialList.empty()) {
		updateMaterialList();
	}	
	// Return the list
	return _materialList;
}
	
}
