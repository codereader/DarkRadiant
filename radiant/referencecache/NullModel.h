#ifndef _NULLMODEL_H_
#define _NULLMODEL_H_

#include "imodel.h"
#include "cullable.h"
#include "math/aabb.h"
#include "entitylib.h"

namespace model {

class NullModel :
	public IModel,
	public Cullable
{
	ShaderPtr _state;
	AABB _aabbLocal;
	RenderableSolidAABB _aabbSolid;
	RenderableWireframeAABB _aabbWire;

	std::string _filename;
	std::string _modelPath;
public:
	NullModel();
	virtual ~NullModel();

	VolumeIntersectionValue intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const;
	
	const AABB& localAABB() const;

	void renderSolid(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;

	void testSelect(Selector& selector, SelectionTest& test, const Matrix4& localToWorld);

	// IModel implementation
	virtual std::string getFilename() const;
	void setFilename(const std::string& filename);

	virtual std::string getModelPath() const;
	void setModelPath(const std::string& modelPath);

	virtual void applySkin(const ModelSkin& skin);

	virtual int getSurfaceCount() const;
	virtual int getVertexCount() const;
	virtual int getPolyCount() const;
	
	virtual const std::vector<std::string>& getActiveMaterials() const;

	// OpenGLRenderable implementation
	void render(RenderStateFlags state) const;
};
typedef boost::shared_ptr<NullModel> NullModelPtr;

} // namespace model

#endif /* _NULLMODEL_H_ */
