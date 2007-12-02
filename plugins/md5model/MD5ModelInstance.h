#ifndef MD5MODELINSTANCE_H_
#define MD5MODELINSTANCE_H_

#include "instancelib.h"
#include "renderable.h"
#include "modelskin.h"
#include "cullable.h"
#include "Bounded.h"
#include "selectable.h"

#include "VectorLightList.h"
#include "MD5Model.h"

namespace md5 {

class MD5ModelInstance :
	public scene::Instance,
	public Renderable,
	public SelectionTestable,
	public LightCullable,
	public SkinnedModel,
	public Bounded,
	public Cullable
{
	// A reference to the actual MD5Model (owned by the Node)
	MD5Model& _model;

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
	MD5ModelInstance(const scene::Path& path, scene::Instance* parent, MD5Model& model);
	~MD5ModelInstance();

	// Bounded implementation
	virtual const AABB& localAABB() const;
	
	// Cullable implementation
	virtual VolumeIntersectionValue intersectVolume(const VolumeTest& test, const Matrix4& localToWorld) const;

	void lightsChanged();
	typedef MemberCaller<MD5ModelInstance, &MD5ModelInstance::lightsChanged> LightsChangedCaller;

	void constructRemaps();
	void destroyRemaps();

	// Returns the name of the currently active skin
	virtual std::string getSkin() const;

	void skinChanged(const std::string& newSkinName);

	void render(Renderer& renderer, const VolumeTest& volume,
			const Matrix4& localToWorld) const;

	void renderSolid(Renderer& renderer, const VolumeTest& volume) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const;

	void testSelect(Selector& selector, SelectionTest& test);

	bool testLight(const RendererLight& light) const;
	void insertLight(const RendererLight& light);
	void clearLights();
};

} // namespace md5

#endif /*MD5MODELINSTANCE_H_*/
