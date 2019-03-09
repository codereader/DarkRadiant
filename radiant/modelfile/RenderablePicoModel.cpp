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
#include "BasicUndoMemento.h"

namespace model
{

// Constructor
RenderablePicoModel::RenderablePicoModel(picoModel_t* mod, const std::string& fExt) :
	_scaleTransformed(1,1,1),
	_scale(1,1,1),
	_undoStateSaver(nullptr),
	_mapFileChangeTracker(nullptr)
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
	_scaleTransformed(other._scaleTransformed),
	_scale(other._scale), // use scale of other model
	_localAABB(other._localAABB),
	_filename(other._filename),
	_modelPath(other._modelPath),
	_undoStateSaver(nullptr),
	_mapFileChangeTracker(nullptr)
{
	// Copy the other model's surfaces, but not its shaders, revert to default
	for (std::size_t i = 0; i < other._surfVec.size(); ++i)
	{
		// Copy-construct the other surface, inheriting any applied scale
		_surfVec[i].surface = std::make_shared<RenderablePicoSurface>(*(other._surfVec[i].surface));
		_surfVec[i].originalSurface = other._surfVec[i].originalSurface;
		_surfVec[i].surface->setActiveMaterial(_surfVec[i].surface->getDefaultMaterial());
	}
}

void RenderablePicoModel::connectUndoSystem(IMapFileChangeTracker& changeTracker)
{
	assert(_undoStateSaver == nullptr);

	// Keep a reference around, we need it when faces are changing
	_mapFileChangeTracker = &changeTracker;

	_undoStateSaver = GlobalUndoSystem().getStateSaver(*this, changeTracker);
}

void RenderablePicoModel::disconnectUndoSystem(IMapFileChangeTracker& changeTracker)
{
	assert(_undoStateSaver != nullptr);

	_mapFileChangeTracker = nullptr;
	_undoStateSaver = nullptr;
	GlobalUndoSystem().releaseStateSaver(*this);
}

void RenderablePicoModel::foreachVisibleSurface(const std::function<void(const Surface& s)>& func) const
{
	for (const Surface& surface : _surfVec)
	{
		assert(surface.shader);

		// Check if the surface's shader is filtered, if not then submit it for rendering
		const MaterialPtr& surfaceShader = surface.shader->getMaterial();

		if (surfaceShader->isVisible())
		{
			func(surface);
		}
	}
}

void RenderablePicoModel::renderSolid(RenderableCollector& rend, const Matrix4& localToWorld,
	const IRenderEntity& entity, const LightList& lights) const
{
	// Submit renderables from each surface
	foreachVisibleSurface([&](const Surface& s)
	{
		// Submit the ordinary shader for material-based rendering
		rend.addRenderable(s.shader, *s.surface, localToWorld, entity, lights);
	});
}

void RenderablePicoModel::renderWireframe(RenderableCollector& rend, const Matrix4& localToWorld,
	const IRenderEntity& entity) const
{
	// Submit renderables from each surface
	foreachVisibleSurface([&](const Surface& s)
	{
		// Submit the wireframe shader for non-shaded renderers
		rend.addRenderable(entity.getWireShader(), *s.surface, localToWorld, entity);
	});
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

std::string RenderablePicoModel::getFilename() const 
{
	return _filename;
}

void RenderablePicoModel::setFilename(const std::string& name)
{
	_filename = name;
}

// Return vertex count of this model
int RenderablePicoModel::getVertexCount() const 
{
	int sum = 0;

	for (const Surface& s : _surfVec)
	{
		sum += s.surface->getNumVertices();
	}

	return sum;
}

// Return poly count of this model
int RenderablePicoModel::getPolyCount() const
{
	int sum = 0;

	for (const Surface& s : _surfVec)
	{
		sum += s.surface->getNumTriangles();
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
		const std::string& activeMaterial = i->surface->getActiveMaterial();

		// Look up the remap for this surface's material name. If there is a remap
		// change the Shader* to point to the new shader.
		std::string remap = skin.getRemap(defaultMaterial);

		if (!remap.empty() && remap != activeMaterial)
		{
			// Save the remapped shader name
			i->surface->setActiveMaterial(remap);
		}
		else if (remap.empty() && activeMaterial != defaultMaterial)
		{
			// No remap, so reset our shader to the original unskinned shader
			i->surface->setActiveMaterial(defaultMaterial);
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
			i->shader = renderSystem->capture(i->surface->getActiveMaterial());
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

	for (const auto& s : _surfVec)
	{
		_materialList.push_back(s.surface->getActiveMaterial());
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

const RenderablePicoModel::SurfaceList& RenderablePicoModel::getSurfaces() const
{
	return _surfVec;
}

std::string RenderablePicoModel::getModelPath() const
{
	return _modelPath;
}

void RenderablePicoModel::setModelPath(const std::string& modelPath)
{
	_modelPath = modelPath;
}

void RenderablePicoModel::revertScale()
{
	_scaleTransformed = _scale;
}

void RenderablePicoModel::evaluateScale(const Vector3& scale)
{
	_scaleTransformed *= scale;

	applyScaleToSurfaces();
}

void RenderablePicoModel::applyScaleToSurfaces()
{
	_localAABB = AABB();

	// Apply the scale to each surface
	for (Surface& surf : _surfVec)
	{
		// Are we still using the original surface? If yes,
		// it's now time to create a working copy
		if (surf.surface == surf.originalSurface)
		{
			// Copy-construct the surface
			surf.surface = std::make_shared<RenderablePicoSurface>(*surf.originalSurface);
		}

		// Apply the scale, on top of the original surface, this should save us from
		// reverting the transformation each time the scale changes
		surf.surface->applyScale(_scaleTransformed, *(surf.originalSurface));

		// Extend the model AABB to include the surface's AABB
		_localAABB.includeAABB(surf.surface->getAABB());
	}
}

// Freeze transform, move the applied scale to the original model
void RenderablePicoModel::freezeScale()
{
	undoSave();

	// Apply the scale to each surface
	_scale = _scaleTransformed;
}

void RenderablePicoModel::undoSave()
{
	if (_undoStateSaver != nullptr)
	{
		_undoStateSaver->save(*this);
	}
}

IUndoMementoPtr RenderablePicoModel::exportState() const
{
	return IUndoMementoPtr(new undo::BasicUndoMemento<Vector3>(_scale));
}

void RenderablePicoModel::importState(const IUndoMementoPtr& state)
{
	undoSave();

	_scale = std::static_pointer_cast< undo::BasicUndoMemento<Vector3> >(state)->data();
	_scaleTransformed = _scale;

	applyScaleToSurfaces();
}

const Vector3& RenderablePicoModel::getScale() const
{
	return _scale;
}

} // namespace
