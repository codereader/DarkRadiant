#include "RenderablePicoModel.h"
#include "RenderablePicoSurface.h"

#include "selectable.h"
#include "texturelib.h"
#include "ishaders.h"
#include "ifilter.h"
#include "math/frustum.h" // VolumeIntersectionValue

namespace model {

// Constructor
RenderablePicoModel::RenderablePicoModel(picoModel_t* mod, 
										 const std::string& fExt) 
{
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
		boost::shared_ptr<RenderablePicoSurface> rSurf(
			new RenderablePicoSurface(surf, fExt));
		_surfVec.push_back(rSurf);
		
		// Extend the model AABB to include the surface's AABB
		_localAABB.includeAABB(rSurf->getAABB());
	}
	
}

// Front end renderable submission
void RenderablePicoModel::submitRenderables(RenderableCollector& rend, 
											const Matrix4& localToWorld)
{
	// Submit renderables from each surface
	for (SurfaceList::iterator i = _surfVec.begin();
		 i != _surfVec.end();
		 ++i)
	{
		// Check if the surface's shader is filtered, if not then submit it for
		// rendering
		MaterialPtr surfaceShader = (*i)->getShader()->getMaterial();
		if (surfaceShader->isVisible()) {		
			(*i)->submitRenderables(rend, localToWorld);
		}
	}
}

// OpenGL (back-end) render function
void RenderablePicoModel::render(const RenderInfo& info) const {
	
	// Render options
	if (info.checkFlag(RENDER_TEXTURE_2D))
		glEnable(GL_TEXTURE_2D);
	if (info.checkFlag(RENDER_SMOOTH))
		glShadeModel(GL_SMOOTH);
	
	// Iterate over the surfaces, calling the render function on each one
	for (SurfaceList::const_iterator i = _surfVec.begin();
		 i != _surfVec.end();
		 ++i)
	{
		// Get the Material to test the shader name against the filter system
		MaterialPtr surfaceShader = (*i)->getShader()->getMaterial();
		if (surfaceShader->isVisible()) {
			// Bind the OpenGL texture and render the surface geometry
			TexturePtr tex = surfaceShader->getEditorImage();
			glBindTexture(GL_TEXTURE_2D, tex->getGLTexNum());
			(*i)->render(info.getFlags());
		}
	}
}

std::string RenderablePicoModel::getFilename() const {
	return _filename;
}

void RenderablePicoModel::setFilename(const std::string& name) {
	_filename = name;
}
	
// Return vertex count of this model
int RenderablePicoModel::getVertexCount() const {
	int sum = 0;
	for (SurfaceList::const_iterator i = _surfVec.begin();
		 i != _surfVec.end();
		 ++i)
	{
		sum += (*i)->getVertexCount();
	}
	return sum;
}

// Return poly count of this model
int RenderablePicoModel::getPolyCount() const {
	int sum = 0;
	for (SurfaceList::const_iterator i = _surfVec.begin();
		 i != _surfVec.end();
		 ++i)
	{
		sum += (*i)->getPolyCount();
	}
	return sum;
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
	
// Perform volume intersection test
VolumeIntersectionValue 
RenderablePicoModel::intersectVolume(const VolumeTest& test,
									 const Matrix4& localToWorld) const
{
	// Simple AABB intersection check between the test volume and the local
	// AABB
	return test.TestAABB(_localAABB, localToWorld);
}

// Perform selection test
void RenderablePicoModel::testSelect(Selector& selector,
									 SelectionTest& test,
									 const Matrix4& localToWorld)
{
	// Perform a volume intersection (AABB) check on each surface. For those
	// that intersect, call the surface's own testSelection method to perform
	// a proper selection test.
    for (SurfaceList::iterator i = _surfVec.begin(); 
    	 i != _surfVec.end(); 
    	 ++i)
	{
		// Check volume intersection
		if ((*i)->intersectVolume(test.getVolume(), localToWorld) 
				!= c_volumeOutside)
		{
			// Volume intersection passed, delegate the selection test
        	(*i)->testSelect(selector, test, localToWorld);
		}
	}
}

std::string RenderablePicoModel::getModelPath() const {
	return _modelPath;
}

void RenderablePicoModel::setModelPath(const std::string& modelPath) {
	_modelPath = modelPath;
}
	
}
