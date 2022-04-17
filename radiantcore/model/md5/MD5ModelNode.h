#pragma once

#include <sigc++/connection.h>
#include "MD5Model.h"
#include "modelskin.h"
#include "itraceable.h"
#include "scene/Node.h"
#include "../RenderableModelSurface.h"
#include "registry/CachedKey.h"
#include "RenderableMD5Skeleton.h"

namespace md5
{

constexpr const char* const RKEY_RENDER_SKELETON = "user/ui/md5/renderSkeleton";

class MD5ModelNode :
	public scene::Node,
	public model::ModelNode,
	public SelectionTestable,
	public SkinnedModel,
	public ITraceable
{
	MD5ModelPtr _model;

	// The name of this model's skin
	std::string _skin;

    // The renderable surfaces attached to the shaders
    std::vector<model::RenderableModelSurface::Ptr> _renderableSurfaces;

    bool _attachedToShaders;

    sigc::connection _animationUpdateConnection;
    sigc::connection _modelShadersChangedConnection;

    registry::CachedKey<bool> _showSkeleton;

    RenderableMD5Skeleton _renderableSkeleton;

public:
	MD5ModelNode(const MD5ModelPtr& model);
    virtual ~MD5ModelNode();

    void onInsertIntoScene(scene::IMapRootNode& root) override;
    void onRemoveFromScene(scene::IMapRootNode& root) override;

	// ModelNode implementation
	const model::IModel& getIModel() const override;
	model::IModel& getIModel() override;
	bool hasModifiedScale() override;
	Vector3 getModelScale() override;

	// returns the contained model
	void setModel(const MD5ModelPtr& model);
	const MD5ModelPtr& getModel() const;

	// Bounded implementation
	virtual const AABB& localAABB() const override;

	virtual std::string name() const override;
	Type getNodeType() const override;

	// SelectionTestable implementation
	void testSelect(Selector& selector, SelectionTest& test) override;

	// Traceable implementation
	bool getIntersection(const Ray& ray, Vector3& intersection) override;

	// Renderable implementation
    void onPreRender(const VolumeTest& volume) override;
	void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override;

	std::size_t getHighlightFlags() override
	{
		return Highlight::NoHighlight; // models are never highlighted themselves
	}

	// Returns the name of the currently active skin
	virtual std::string getSkin() const override;
	void skinChanged(const std::string& newSkinName) override;

    void transformChangedLocal() override;

protected:
    void onVisibilityChanged(bool isVisibleNow) override;

private:
    void onModelAnimationUpdated();
    void attachToShaders();
    void detachFromShaders();
    void onModelShadersChanged();
};

} // namespace
