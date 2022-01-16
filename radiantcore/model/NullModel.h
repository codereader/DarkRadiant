#pragma once

#include "imodel.h"
#include "math/AABB.h"
#include "entitylib.h"

namespace model {

class NullModel :
	public IModel
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

	const AABB& localAABB() const;

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
	virtual const IModelSurface& getSurface(unsigned surfaceNum) const;

	virtual const std::vector<std::string>& getActiveMaterials() const;
};
typedef std::shared_ptr<NullModel> NullModelPtr;

} // namespace model
