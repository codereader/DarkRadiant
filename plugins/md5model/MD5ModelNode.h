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
	virtual const model::IModel& getIModel() const;
	virtual model::IModel& getIModel();

	void lightsChanged();

	// returns the contained model
	void setModel(const MD5ModelPtr& model);
	const MD5ModelPtr& getModel() const;

	// Bounded implementation
	virtual const AABB& localAABB() const;

	virtual std::string name() const;
	Type getNodeType() const;

	// SelectionTestable implementation
	void testSelect(Selector& selector, SelectionTest& test);

	// Traceable implementation
	bool getIntersection(const Ray& ray, Vector3& intersection);

	// LitObject implementation
	bool intersectsLight(const RendererLight& light) const;
	void insertLight(const RendererLight& light);
	void clearLights();

	// Renderable implementation
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;
	void setRenderSystem(const RenderSystemPtr& renderSystem);

	bool isHighlighted() const
	{
		return false; // models are never highlighted themselves
	}

	// Returns the name of the currently active skin
	virtual std::string getSkin() const;
	void skinChanged(const std::string& newSkinName);

private:
	void render(RenderableCollector& collector, const VolumeTest& volume, 
				const Matrix4& localToWorld, const IRenderEntity& entity) const;
};
typedef boost::shared_ptr<MD5ModelNode> MD5ModelNodePtr;

} // namespace md5
