#pragma once

#include "Transformable.h"
#include "iselectiontest.h"
#include "irender.h"
#include "itraceable.h"
#include "modelskin.h"
#include "irenderable.h"
#include "pivot.h"
#include "StaticModel.h"
#include "scene/Node.h"
#include "RenderableModelSurface.h"

namespace model
{

/**
 * \brief Scenegraph node representing a static model loaded from a file (e.g.
 * LWO or ASE).
 *
 * This node does not represent a "func_static" (or similar object) directly,
 * but is added as a child of the respective entity node (e.g.
 * StaticGeometryNode). It is normally created by the ModelCache in response to
 * a particular entity gaining a "model" spawnarg.
 */
class StaticModelNode final :
	public scene::Node,
	public ModelNode,
	public SelectionTestable,
	public SkinnedModel,
	public ITraceable,
    public Transformable
{
private:
	// The actual model
	StaticModelPtr _model;

	std::string _name;

	// The name of this model's skin
	std::string _skin;

    // The renderable surfaces attached to the shaders
    std::vector<RenderableModelSurface::Ptr> _renderableSurfaces;

    // We need to keep a reference for skin swapping
    RenderSystemWeakPtr _renderSystem;

    bool _attachedToShaders;

public:
    typedef std::shared_ptr<StaticModelNode> Ptr;

	// Construct a StaticModelNode with a reference to the loaded StaticModel.
	StaticModelNode(const StaticModelPtr& picoModel);

	void onInsertIntoScene(scene::IMapRootNode& root) override;
	void onRemoveFromScene(scene::IMapRootNode& root) override;

	// ModelNode implementation
	const IModel& getIModel() const override;
	IModel& getIModel() override;
	bool hasModifiedScale() override;
	Vector3 getModelScale() override;

	// SkinnedModel implementation
	// Skin changed notify
	void skinChanged(const std::string& newSkinName) override;
	// Returns the name of the currently active skin
	std::string getSkin() const override;

	// Bounded implementation
	const AABB& localAABB() const override;

	// SelectionTestable implementation
	void testSelect(Selector& selector, SelectionTest& test) override;

	std::string name() const override;
	Type getNodeType() const override;

	const StaticModelPtr& getModel() const;
	void setModel(const StaticModelPtr& model);

	// Renderable implementation
  	void onPreRender(const VolumeTest& volume) override;
	void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override;
	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

	std::size_t getHighlightFlags() override
	{
		return Highlight::NoHighlight; // models are never highlighted themselves
	}

	// Traceable implementation
	bool getIntersection(const Ray& ray, Vector3& intersection) override;

    void transformChangedLocal() override;

protected:
	void _onTransformationChanged() override;
	void _applyTransformation() override;
    void onVisibilityChanged(bool isVisibleNow) override;

private:
    void attachToShaders();
    void detachFromShaders();
    void queueRenderableUpdate();
};

} // namespace model
