#include "NullModel.h"

#include <stdexcept>

namespace model
{

NullModel::NullModel() :
	_aabbLocal(Vector3(0, 0, 0), Vector3(8, 8, 8))
{}

const AABB& NullModel::localAABB() const
{
	return _aabbLocal;
}

std::string NullModel::getFilename() const
{
	return _filename;
}

void NullModel::setFilename(const std::string& filename)
{
	_filename = filename;
}

std::string NullModel::getModelPath() const
{
	return _modelPath;
}

void NullModel::setModelPath(const std::string& modelPath)
{
	_modelPath = modelPath;
}

void NullModel::applySkin(const ModelSkin& skin)
{
	// do nothing
}

int NullModel::getSurfaceCount() const
{
	return 0;
}

int NullModel::getVertexCount() const
{
	return 0;
}

int NullModel::getPolyCount() const
{
	return 0;
}

const IModelSurface& NullModel::getSurface(unsigned surfaceNum) const
{
	throw std::runtime_error("NullModel::getSurface: invalid call, no surfaces.");
}

const StringList& NullModel::getActiveMaterials() const
{
	static std::vector<std::string> _dummyMaterials;
	return _dummyMaterials;
}

} // namespace model
