#pragma once

#include "iundo.h"
#include "imodel.h"
#include "math/AABB.h"
#include "imodelsurface.h"

#include <memory>

class Ray;

/* FORWARD DECLS */
namespace model
{
	class StaticModelSurface;
	typedef std::shared_ptr<StaticModelSurface> StaticModelSurfacePtr;
}
class IRenderableCollector;
class RendererLight;
class SelectionTest;
class Selector;

namespace model
{

/**
 * \brief
 * IModel implementing class representing a static model
 *
 * A StaticModel is made up of one or more StaticModelSurface
 * objects, each of which contains a number of polygons with the same texture.
 * Rendering a StaticModel involves rendering all of its surfaces,
 * submitting their geometry via OpenGL calls.
 */
class StaticModel : 
	public IModel,
	public IUndoable
{
private:
	// greebo: StaticModelSurfaces are shared objects, the actual shaders 
	// and the model skin handling are managed by the nodes/imodels referencing them
	struct Surface
	{
		// The (shared) surface object
		StaticModelSurfacePtr surface;

		// The (unmodified) surface object
		StaticModelSurfacePtr originalSurface;

		// The shader this surface is using
		ShaderPtr shader;

		Surface()
		{}

		// Constructor
		Surface(const StaticModelSurfacePtr& surface_) :
			surface(surface_),
			originalSurface(surface)
		{}
	};

	typedef std::vector<Surface> SurfaceList;

	// Vector of renderable surfaces for this model
	SurfaceList _surfaces;

	// The current working scale
	Vector3 _scaleTransformed;

	// The scale for this model (is 1,1,1 for an unmodified one)
	Vector3 _scale;

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

	// Undoable stuff
	IUndoStateSaver* _undoStateSaver;

    sigc::signal<void> _sigShadersChanged;
    sigc::signal<void> _sigSurfaceScaleApplied;

private:

	// Update the list of materials by querying each surface for its current
	// material.
	void updateMaterialList() const;

	// Ensure all shaders for the active materials
	void captureShaders();

	void undoSave();

	void applyScaleToSurfaces();

	void foreachVisibleSurface(const std::function<void(const Surface& s)>& func) const;

public:

    /**
     * Construct a StaticModel with the given set of surfaces.
     */
    StaticModel(const std::vector<StaticModelSurfacePtr>& surfaces);

	/**
	 * Copy constructor: re-use the surfaces from the other model
	 * but make it possible to assign custom skins to the surfaces.
	 */
	StaticModel(const StaticModel& other);

	void connectUndoSystem(IUndoSystem& undoSystem);
	void disconnectUndoSystem(IUndoSystem& undoSystem);

	void setRenderSystem(const RenderSystemPtr& renderSystem);

    // A signal that is emitted after the captured shaders have been changed (or cleared)
    sigc::signal<void>& signal_ShadersChanged();

    // Signal emitted when any surface scale has been changed
    sigc::signal<void>& signal_SurfaceScaleApplied();

	/**
	 * Return the number of surfaces in this model.
	 */
	int getSurfaceCount() const override
    {
		return static_cast<int>(_surfaces.size());
	}

	/**
	 * Return the number of vertices in this model, by summing the vertex
	 * counts for each surface.
	 */
	int getVertexCount() const override;

	/** Return the polycount (tricount) of this model by summing the surface
	 * polycounts.
	 */

	int getPolyCount() const override;

	const IModelSurface& getSurface(unsigned surfaceNum) const override;

	/**
	 * Return the enclosing AABB for this model.
	 */
	const AABB& localAABB() const override
    {
		return _localAABB;
	}

	/** Return the list of active materials for this model.
	 */
	const std::vector<std::string>& getActiveMaterials() const override;

	// Sets the filename this model was loaded from
	void setFilename(const std::string& name);

	// Returns the filename this model was loaded from
	virtual std::string getFilename() const override;

	// Returns the VFS path to the model file
	virtual std::string getModelPath() const override;

	void setModelPath(const std::string& modelPath);

	/** Apply the given skin to this model.
	 */
	void applySkin(const ModelSkin& skin) override;

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
	 * Return the list of StaticModelSurface objects.
	 */
	const SurfaceList& getSurfaces() const;

	// Revert to base scale (returns true if the scale was actually changed by this call)
	bool revertScale();

	// TransformationChanged, apply the given scale to the "working copy"
	void evaluateScale(const Vector3& scale);

	// Freeze transform, store new base scale
	void freezeScale();

	// Undoable implementation
	IUndoMementoPtr exportState() const override;
	void importState(const IUndoMementoPtr& state) override;

	// Returns the current base scale of this model
	const Vector3& getScale() const;

    void foreachSurface(const std::function<void(const StaticModelSurface&)>& func) const;
};
typedef std::shared_ptr<StaticModel> StaticModelPtr;

}
