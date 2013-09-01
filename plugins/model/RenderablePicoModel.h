#pragma once

#include "imodel.h"
#include "picomodel.h"
#include "math/AABB.h"
#include "imodelsurface.h"

#include <boost/shared_ptr.hpp>

class Ray;

/* FORWARD DECLS */
namespace model
{
	class RenderablePicoSurface;
	typedef boost::shared_ptr<RenderablePicoSurface> RenderablePicoSurfacePtr;
}
class RenderableCollector;
class RendererLight;
class SelectionTest;
class Selector;

namespace model
{

/**
 * Renderable class containing a model loaded via the picomodel library. A
 * RenderablePicoModel is made up of one or more RenderablePicoSurface objects,
 * each of which contains a number of polygons with the same texture. Rendering
 * a RenderablePicoModel involves rendering all of its surfaces, submitting 
 * their geometry via OpenGL calls.
 */
class RenderablePicoModel
: public IModel
{
private:
	// greebo: RenderablePicoSurfaces are shared objects, the actual shaders 
	// and the model skin handling are managed by the nodes/imodels referencing them
	struct Surface
	{
		// The (shared) surface object
		RenderablePicoSurfacePtr surface;

		// The name of the material (with skin applied)
		// The default material name is stored on the surface
		std::string activeMaterial;

		// The shader this surface is using
		ShaderPtr shader;

		Surface()
		{}

		// Constructor
		Surface(const RenderablePicoSurfacePtr& surface_) :
			surface(surface_)
		{}
	};

	// List of RenderablePicoSurfaces.
	typedef std::vector<Surface> SurfaceList;

	// Vector of renderable surfaces for this model
	SurfaceList _surfVec;

	// Local AABB for this model
	AABB _localAABB;

	// Vector of materials used by this model (one for each surface)
	mutable std::vector<std::string> _materialList;

	// The filename this model was loaded from
	std::string _filename;

	// The VFS path to this model
	std::string _modelPath;

	// We need to keep a reference for skin swapping
	RenderSystemWeakPtr _renderSystem;

private:

	// Update the list of materials by querying each surface for its current
	// material.
	void updateMaterialList() const;

	// Ensure all shaders for the active materials
	void captureShaders();

public:

	/**
	 * Constructor. Accepts a picoModel_t struct containing the raw model data
	 * loaded from picomodel, and a string filename extension to allow the
	 * correct handling of material paths (which differs between ASE and LWO)
	 */
	RenderablePicoModel(picoModel_t* mod, const std::string& fExt);

	/**
	 * Copy constructor: re-use the surfaces from the other model
	 * but make it possible to assign custom skins to the surfaces.
	 */
	RenderablePicoModel(const RenderablePicoModel& other);

	/**
	 * Front-end render function used by the main collector.
	 *
	 * @param rend
	 * The sorting RenderableCollector object which accepts renderable geometry.
	 *
	 * @param localToWorld
	 * Object to world-space transform.
	 *
	 * @param entity
	 * The entity this model is attached to.
	 */
	void submitRenderables(RenderableCollector& rend, const Matrix4& localToWorld,
						   const IRenderEntity& entity);

	void setRenderSystem(const RenderSystemPtr& renderSystem);

	/**
	 * Back-end render function from OpenGLRenderable. This is called from the
	 * model selector but not the main renderer, which uses the front-end render
	 * method.
	 */
	void render(const RenderInfo& info) const;

	/**
	 * Return the number of surfaces in this model.
	 */
	int getSurfaceCount() const {
		return static_cast<int>(_surfVec.size());
	}

	/**
	 * Return the number of vertices in this model, by summing the vertex
	 * counts for each surface.
	 */
	int getVertexCount() const;

	/** Return the polycount (tricount) of this model by summing the surface
	 * polycounts.
	 */

	int getPolyCount() const;

	const IModelSurface& getSurface(unsigned surfaceNum) const;

	/**
	 * Return the enclosing AABB for this model.
	 */
	const AABB& localAABB() const {
		return _localAABB;
	}

	/** Return the list of active materials for this model.
	 */
	const std::vector<std::string>& getActiveMaterials() const;

	// Sets the filename this model was loaded from
	void setFilename(const std::string& name);

	// Returns the filename this model was loaded from
	virtual std::string getFilename() const;

	// Returns the VFS path to the model file
	virtual std::string getModelPath() const;

	void setModelPath(const std::string& modelPath);

	/** Apply the given skin to this model.
	 */
	void applySkin(const ModelSkin& skin);

	/**
	 * Selection test. Test each surface against the SelectionTest object and
	 * if the surface is selected, add it to the selector.
	 *
	 * @param selector
	 * Selector object which builds a list of selectables.
	 *
	 * @param test
	 * The SelectionTest object defining the 3D properties of the selection.
	 *
	 * @param localToWorld
	 * Object to world space transform.
	 */
	void testSelect(Selector& selector,
					SelectionTest& test,
					const Matrix4& localToWorld);

	// Returns true if the given ray intersects this model geometry and fills in
	// the exact point in the given Vector3, returns false if no intersection was found.
	bool getIntersection(const Ray& ray, Vector3& intersection, const Matrix4& localToWorld);

	/**
	 * Return the list of RenderablePicoSurface objects.
	 */
	const SurfaceList& getSurfaces() const {
		return _surfVec;
	}
};
typedef boost::shared_ptr<RenderablePicoModel> RenderablePicoModelPtr;

}
