#pragma once

#include "iselectiontest.h"
#include "itraceable.h"
#include "modelskin.h"
#include "irenderable.h"

#include "ModelNodeBase.h"
#include "Transformable.h"
#include "StaticModel.h"

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
	public ModelNodeBase,
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

	const StaticModelPtr& getModel() const;
	void setModel(const StaticModelPtr& model);

	// Renderable implementation
	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

	// Traceable implementation
	bool getIntersection(const Ray& ray, Vector3& intersection) override;

    // Called when the contained model has applied the scale to its surfaces
    // The Node listens to this and queues a renderable update
    void onModelScaleApplied();

protected:
    void createRenderableSurfaces() override;

	void _onTransformationChanged() override;
	void _applyTransformation() override;

private:
    void onModelShadersChanged();
};

} // namespace model
