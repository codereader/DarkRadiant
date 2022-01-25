#pragma once

#include "imodel.h"
#include "math/AABB.h"

namespace model
{

class NullModel final :
	public IModel
{
	AABB _aabbLocal;

	std::string _filename;
	std::string _modelPath;
public:
	NullModel();

	const AABB& localAABB() const override;

	// IModel implementation
	std::string getFilename() const override;
	void setFilename(const std::string& filename);

	std::string getModelPath() const override;
	void setModelPath(const std::string& modelPath);

	void applySkin(const ModelSkin& skin) override;

	int getSurfaceCount() const override;
    int getVertexCount() const override;
	int getPolyCount() const override;
	const IModelSurface& getSurface(unsigned surfaceNum) const override;

	const std::vector<std::string>& getActiveMaterials() const override;
};
typedef std::shared_ptr<NullModel> NullModelPtr;

} // namespace model
