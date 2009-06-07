#include "NullModel.h"

#include "math/frustum.h"
#include "irenderable.h"

namespace model {

NullModel::NullModel() : 
	_aabbLocal(Vector3(0, 0, 0), Vector3(8, 8, 8)), 
	_aabbSolid(_aabbLocal), 
	_aabbWire(_aabbLocal)
{
	_state = GlobalRenderSystem().capture("");
}

NullModel::~NullModel() {
	_state = ShaderPtr();
}

VolumeIntersectionValue NullModel::intersectVolume(
	const VolumeTest& volume, const Matrix4& localToWorld) const
{
	return volume.TestAABB(_aabbLocal, localToWorld);
}

const AABB& NullModel::localAABB() const {
	return _aabbLocal;
}

void NullModel::renderSolid(RenderableCollector& collector, 
	const VolumeTest& volume, const Matrix4& localToWorld) const
{
	collector.SetState(_state, RenderableCollector::eFullMaterials);
	collector.addRenderable(_aabbSolid, localToWorld);
}

void NullModel::renderWireframe(RenderableCollector& collector, 
	const VolumeTest& volume, const Matrix4& localToWorld) const
{
	collector.SetState(_state, RenderableCollector::eWireframeOnly);
	collector.addRenderable(_aabbWire, localToWorld);
}

void NullModel::testSelect(Selector& selector, SelectionTest& test, const Matrix4& localToWorld) {
	test.BeginMesh(localToWorld);

	SelectionIntersection best;
	aabb_testselect(_aabbLocal, test, best);

	if(best.valid()) {
		selector.addIntersection(best);
	}
}

std::string NullModel::getFilename() const {
	return _filename;
}

void NullModel::setFilename(const std::string& filename) {
	_filename = filename;
}

std::string NullModel::getModelPath() const {
	return _modelPath;
}

void NullModel::setModelPath(const std::string& modelPath) {
	_modelPath = modelPath;
}

void NullModel::applySkin(const ModelSkin& skin) {
	// do nothing
}

int NullModel::getSurfaceCount() const {
	return 0;
}

int NullModel::getVertexCount() const {
	return 0;
}

int NullModel::getPolyCount() const {
	return 0;
}

const MaterialList& NullModel::getActiveMaterials() const {
	static std::vector<std::string> _dummyMaterials;
	return _dummyMaterials;
}

void NullModel::render(const RenderInfo& info) const {
	if (info.checkFlag(RENDER_TEXTURE_2D)) 
    {
		aabb_draw_solid(_aabbLocal, info.getFlags());
	}
	else 
    {
		aabb_draw_wire(_aabbLocal);
	}
}

} // namespace model
