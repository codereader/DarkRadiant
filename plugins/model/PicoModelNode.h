#ifndef PICOMODELNODE_H_
#define PICOMODELNODE_H_

#include "scenelib.h"
#include "selectable.h"
#include "nameable.h"
#include "irender.h"
#include "modelskin.h"
#include "irenderable.h"
#include "VectorLightList.h"
#include "RenderablePicoModel.h"

namespace model {

class PicoModelNode : 
	public scene::Node, 
	public ModelNode,
	public Nameable,
	public SelectionTestable,
	public LightCullable,
	public Renderable,
	public Cullable,
	public Bounded,
	public SkinnedModel
{
	// The actual model
	RenderablePicoModelPtr _picoModel;

	std::string _name;

	// Vector of RendererLight references which illuminate this instance, set
	// with addLight() and clearLights()
	VectorLightList _lights;

	// The light list from the shader cache when we attach
	const LightList& _lightList;

	// Cache of RenderablePicoSurfaces along with their shaders. This is 
	// necessary to allow each Instance to have its own skin.
	typedef std::pair< boost::shared_ptr<RenderablePicoSurface>,
					   ShaderPtr> MappedSurface;
	typedef std::vector<MappedSurface> MappedSurfaces;
	MappedSurfaces _mappedSurfs;

	// The name of this model's skin
	std::string _skin;

public:
	/** Construct a PicoModelNode with a reference to the loaded picoModel.
	 */
	PicoModelNode(const RenderablePicoModelPtr& picoModel);

	virtual ~PicoModelNode();

	// ModelNode implementation
	virtual const IModel& getIModel() const;

	// SkinnedModel implementation
	// Skin changed notify
	void skinChanged(const std::string& newSkinName);
	// Returns the name of the currently active skin
	std::string getSkin() const;

	// Bounded implementation
	virtual const AABB& localAABB() const;

	// Lights changed function
	void lightsChanged() {
		_lightList.lightsChanged();
	}
	typedef MemberCaller<PicoModelNode, &PicoModelNode::lightsChanged> LightsChangedCaller;

	// scene::Instantiable implementation
	virtual void instantiate(const scene::Path& path);
	virtual void uninstantiate(const scene::Path& path);

	// SelectionTestable implementation
	void testSelect(Selector& selector, SelectionTest& test);

	// Cullable implementation
	virtual VolumeIntersectionValue intersectVolume(
		const VolumeTest& test, const Matrix4& localToWorld) const;

	virtual std::string name() const;
  
	const RenderablePicoModelPtr& getModel() const;
	void setModel(const RenderablePicoModelPtr& model);

	// LightCullable test function
	bool testLight(const RendererLight& light) const;
	// Add a light to this model instance
	void insertLight(const RendererLight& light);
	// Clear all lights from this model instance
	void clearLights();

	// Renderable implementation	
  	void renderSolid(Renderer& renderer, const VolumeTest& volume) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const;

private:
	// Instance render function 
	void submitRenderables(Renderer& renderer, 
						   const VolumeTest& volume, 
						   const Matrix4& localToWorld) const;
};
typedef boost::shared_ptr<PicoModelNode> PicoModelNodePtr;

} // namespace model

#endif /* PICOMODELNODE_H_ */
