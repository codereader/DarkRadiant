#pragma once

#include "MD5Model.h"
#include "modelskin.h"
#include "itraceable.h"
#include "scene/Node.h"
#include "render/VectorLightList.h"

namespace md5 {

class MD5ModelNode :
	public scene::Node,
	public model::ModelNode,
	public SelectionTestable,
	public LitObject,
	public SkinnedModel,
	public ITraceable
{
private:
	MD5ModelPtr _model;

	LightList* _lightList;

	typedef std::vector<render::lib::VectorLightList> SurfaceLightLists;
	SurfaceLightLists _surfaceLightLists;

	// The name of this model's skin
	std::string _skin;

public:
	MD5ModelNode(const MD5ModelPtr& model);
	virtual ~MD5ModelNode();

	// ModelNode implementation
	const model::IModel& getIModel() const override;
	model::IModel& getIModel() override;
	bool hasModifiedScale() override;

	void lightsChanged();

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

	// LitObject implementation
	bool intersectsLight(const RendererLight& light) const override;
	void insertLight(const RendererLight& light) override;
	void clearLights() override;

	// Renderable implementation
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override;
	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

	std::size_t getHighlightFlags() override
	{
		return Highlight::NoHighlight; // models are never highlighted themselves
	}

	// Returns the name of the currently active skin
	virtual std::string getSkin() const override;
	void skinChanged(const std::string& newSkinName) override;

private:
	void render(RenderableCollector& collector, const VolumeTest& volume, 
				const Matrix4& localToWorld, const IRenderEntity& entity) const;
};
typedef std::shared_ptr<MD5ModelNode> MD5ModelNodePtr;

} // namespace md5
