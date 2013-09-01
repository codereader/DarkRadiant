#include "RenderablePicoModel.h"
#include "RenderablePicoSurface.h"

#include "ivolumetest.h"
#include "iselectiontest.h"
#include "texturelib.h"
#include "ishaders.h"
#include "modelskin.h"
#include "ifilter.h"
#include "imodelsurface.h"
#include "VolumeIntersectionValue.h"
#include "math/Ray.h"

namespace model
{

// Constructor
RenderablePicoModel::RenderablePicoModel(picoModel_t* mod,
										 const std::string& fExt)
{
	// Get the number of surfaces to create
	int nSurf = PicoGetModelNumSurfaces(mod);

	// Create a RenderablePicoSurface for each surface in the structure
	for (int n = 0; n < nSurf; ++n)
	{
		// Retrieve the surface, discarding it if it is null or non-triangulated (?)
		picoSurface_t* surf = PicoGetModelSurface(mod, n);

		if (surf == 0 || PicoGetSurfaceType(surf) != PICO_TRIANGLES)
			continue;

		// Fix the normals of the surface (?)
		PicoFixSurfaceNormals(surf);

		// Create the RenderablePicoSurface object and add it to the vector
		RenderablePicoSurfacePtr rSurf(new RenderablePicoSurface(surf, fExt));

		_surfVec.push_back(Surface(rSurf));

		// Extend the model AABB to include the surface's AABB
		_localAABB.includeAABB(rSurf->getAABB());
	}
}

RenderablePicoModel::RenderablePicoModel(const RenderablePicoModel& other) :
	_surfVec(other._surfVec.size()),
	_localAABB(other._localAABB),
	_filename(other._filename),
	_modelPath(other._modelPath)
{
	// Copy the other model's surfaces, but not its shaders, revert to default
	for (std::size_t i = 0; i < other._surfVec.size(); ++i)
	{
		_surfVec[i].surface = other._surfVec[i].surface;
		_surfVec[i].activeMaterial = _surfVec[i].surface->getDefaultMaterial();
	}
}

// Front end renderable submission
void RenderablePicoModel::submitRenderables(RenderableCollector& rend,
											const Matrix4& localToWorld,
											const IRenderEntity& entity)
{
	// Submit renderables from each surface
	for (SurfaceList::iterator i = _surfVec.begin(); i != _surfVec.end(); ++i)
	{
		assert(i->shader);

		// Check if the surface's shader is filtered, if not then submit it for
		// rendering
		const MaterialPtr& surfaceShader = i->shader->getMaterial();

		if (surfaceShader->isVisible())
		{
			i->surface->submitRenderables(rend, localToWorld, i->shader, entity);
		}
	}
}

void RenderablePicoModel::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	_renderSystem = renderSystem;

	captureShaders();
}

// OpenGL (back-end) render function
void RenderablePicoModel::render(const RenderInfo& info) const
{
// greebo: No GL state changes in render methods!
#if 0
	// Render options
	if (info.checkFlag(RENDER_TEXTURE_2D))
	{
		glEnable(GL_TEXTURE_2D);
	}

	if (info.checkFlag(RENDER_SMOOTH))
	{
		glShadeModel(GL_SMOOTH);
	}
#endif

	// Iterate over the surfaces, calling the render function on each one
	for (SurfaceList::const_iterator i = _surfVec.begin();
		 i != _surfVec.end();
		 ++i)
	{
// greebo: Shader visibility checks have already been performed in the front end pass
#if 0
		// Get the Material to test the shader name against the filter system
		const MaterialPtr& surfaceShader = i->shader->getMaterial();

		if (surfaceShader->isVisible())
		{
			// Bind the OpenGL texture and render the surface geometry
			TexturePtr tex = surfaceShader->getEditorImage();
			glBindTexture(GL_TEXTURE_2D, tex->getGLTexNum());
			i->surface->render(info.getFlags());
		}
#else
		i->surface->render(info.getFlags());
#endif
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
		sum += i->surface->getNumVertices();
	}
	return sum;
}

// Return poly count of this model
int RenderablePicoModel::getPolyCount() const
{
	int sum = 0;

	for (SurfaceList::const_iterator i = _surfVec.begin();
		 i != _surfVec.end();
		 ++i)
	{
		sum += i->surface->getNumTriangles();
	}

	return sum;
}

const IModelSurface& RenderablePicoModel::getSurface(unsigned surfaceNum) const
{
	assert(surfaceNum >= 0 && surfaceNum < _surfVec.size());
	return *(_surfVec[surfaceNum].surface);
}

// Apply the given skin to this model
void RenderablePicoModel::applySkin(const ModelSkin& skin)
{
	// Apply the skin to each surface, then try to capture shaders
	for (SurfaceList::iterator i = _surfVec.begin();
		 i != _surfVec.end();
		 ++i)
	{
		const std::string& defaultMaterial = i->surface->getDefaultMaterial();
		const std::string& activeMaterial = i->activeMaterial;

		// Look up the remap for this surface's material name. If there is a remap
		// change the Shader* to point to the new shader.
		std::string remap = skin.getRemap(defaultMaterial);

		if (!remap.empty() && remap != activeMaterial)
		{
			// Save the remapped shader name
			i->activeMaterial = remap;
		}
		else if (remap.empty() && activeMaterial != defaultMaterial)
		{
			// No remap, so reset our shader to the original unskinned shader
			i->activeMaterial = defaultMaterial;
		}
	}

	captureShaders();

	// greebo: Update the active material list after applying this skin
	updateMaterialList();
}

void RenderablePicoModel::captureShaders()
{
	RenderSystemPtr renderSystem = _renderSystem.lock();

	// Capture or release our shaders
	for (SurfaceList::iterator i = _surfVec.begin(); i != _surfVec.end(); ++i)
	{
		if (renderSystem)
		{
			i->shader = renderSystem->capture(i->activeMaterial);
		}
		else
		{
			i->shader.reset();
		}
	}
}

// Update the list of active materials
void RenderablePicoModel::updateMaterialList() const
{
	_materialList.clear();

	for (SurfaceList::const_iterator i = _surfVec.begin();
		 i != _surfVec.end();
		 ++i)
	{
		_materialList.push_back(i->activeMaterial);
	}
}

// Return the list of active skins for this model
const StringList& RenderablePicoModel::getActiveMaterials() const
{
	// If the material list is empty, populate it
	if (_materialList.empty())
	{
		updateMaterialList();
	}

	// Return the list
	return _materialList;
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
		if (test.getVolume().TestAABB(i->surface->getAABB(), localToWorld) != VOLUME_OUTSIDE)
		{
			// Volume intersection passed, delegate the selection test
        	i->surface->testSelect(selector, test, localToWorld);
		}
	}
}

bool RenderablePicoModel::getIntersection(const Ray& ray, Vector3& intersection, const Matrix4& localToWorld)
{
	Vector3 bestIntersection = ray.origin;

	// Test each surface and take the nearest point to the ray origin
	for (SurfaceList::iterator i = _surfVec.begin(); i != _surfVec.end(); ++i)
	{
		Vector3 surfaceIntersection;

		if (i->surface->getIntersection(ray, surfaceIntersection, localToWorld))
		{
			// Test if this surface intersection is better than what we currently have
			float oldDistSquared = (bestIntersection - ray.origin).getLengthSquared();
			float newDistSquared = (surfaceIntersection - ray.origin).getLengthSquared();

			if ((oldDistSquared == 0 && newDistSquared > 0) || newDistSquared < oldDistSquared)
			{
				bestIntersection = surfaceIntersection;
			}
		}
	}

	if ((bestIntersection - ray.origin).getLengthSquared() > 0)
	{
		intersection = bestIntersection;
		return true;
	}
	else
	{
		return false;
	}
}

std::string RenderablePicoModel::getModelPath() const
{
	return _modelPath;
}

void RenderablePicoModel::setModelPath(const std::string& modelPath)
{
	_modelPath = modelPath;
}

} // namespace
