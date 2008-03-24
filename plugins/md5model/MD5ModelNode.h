#ifndef MD5MODELNODE_H_
#define MD5MODELNODE_H_

#include "scenelib.h"
#include "nameable.h"
#include "MD5Model.h"
#include "modelskin.h"
#include "VectorLightList.h"

namespace md5 {

class MD5ModelNode : 
	public scene::Node,
	public model::ModelNode,
	public Nameable,
	public SelectionTestable,
	public LightCullable,
	public Renderable,
	public Cullable,
	public Bounded,
	public SkinnedModel
{
	MD5ModelPtr _model;

	const LightList* _lightList;

	typedef Array<VectorLightList> SurfaceLightLists;
	SurfaceLightLists _surfaceLightLists;

	struct Remap {
		std::string name;
		ShaderPtr shader;
	};
  
	typedef Array<Remap> SurfaceRemaps;
	SurfaceRemaps _surfaceRemaps;

	// The name of this model's skin
	std::string _skin;
	
public:
	MD5ModelNode(const MD5ModelPtr& model);
	virtual ~MD5ModelNode();

	// ModelNode implementation
	virtual const model::IModel& getIModel() const;

	void lightsChanged();
	typedef MemberCaller<MD5ModelNode, &MD5ModelNode::lightsChanged> LightsChangedCaller;

	// returns the contained model
	void setModel(const MD5ModelPtr& model);
	const MD5ModelPtr& getModel() const;

	// Bounded implementation
	virtual const AABB& localAABB() const;

	// scene::Instantiable implementation
	virtual void instantiate(const scene::Path& path);
	virtual void uninstantiate(const scene::Path& path);

	// Cullable implementation
	virtual VolumeIntersectionValue intersectVolume(const VolumeTest& test, const Matrix4& localToWorld) const;

	// Nameable implementation
	virtual std::string name() const;

	// SelectionTestable implementation
	void testSelect(Selector& selector, SelectionTest& test);

	// LightCullable implementation
	bool testLight(const RendererLight& light) const;
	void insertLight(const RendererLight& light);
	void clearLights();

	// Renderable implementation
	void renderSolid(Renderer& renderer, const VolumeTest& volume) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const;

	// Returns the name of the currently active skin
	virtual std::string getSkin() const;
	void skinChanged(const std::string& newSkinName);

private:
	void constructRemaps();
	void destroyRemaps();

	void render(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;
};
typedef boost::shared_ptr<MD5ModelNode> MD5ModelNodePtr;

} // namespace md5

#endif /*MD5MODELNODE_H_*/
