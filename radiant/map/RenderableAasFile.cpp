#include "RenderableAasFile.h"

#include "iregistry.h"
#include "imainframe.h"

#include "registry/registry.h"

namespace map
{

RenderableAasFile::RenderableAasFile() :
	_renderNumbers(registry::getValue<bool>(RKEY_SHOW_AAS_AREA_NUMBERS))
{
	GlobalRegistry().signalForKey(RKEY_SHOW_AAS_AREA_NUMBERS).connect([this]()
	{
		_renderNumbers = registry::getValue<bool>(RKEY_SHOW_AAS_AREA_NUMBERS);
		GlobalMainFrame().updateAllWindows();
	});
}

void RenderableAasFile::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	_renderSystem = renderSystem;
}

void RenderableAasFile::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	if (!_aasFile) return;

	collector.SetState(_normalShader, RenderableCollector::eFullMaterials);

	for (const RenderableSolidAABB& aabb : _renderableAabbs)
	{
		collector.addRenderable(aabb, Matrix4::getIdentity());
	}

	if (_renderNumbers)
	{
		collector.addRenderable(*this, Matrix4::getIdentity());
	}
}

void RenderableAasFile::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	// Do nothing in wireframe mode
	//renderSolid(collector, volume);
}

std::size_t RenderableAasFile::getHighlightFlags()
{
	return Highlight::None;
}

void RenderableAasFile::setAasFile(const IAasFilePtr& aasFile)
{
	_aasFile = aasFile;

	prepare();
}

void RenderableAasFile::render(const RenderInfo& info) const
{
	// draw label
	// Render the area numbers
	for (std::size_t areaNum = 0; areaNum < _aasFile->getNumAreas(); ++areaNum)
	{
		const IAasFile::Area& area = _aasFile->getArea(areaNum);

		glRasterPos3dv(area.center);
		GlobalOpenGL().drawString(string::to_string(areaNum));
	}
}

void RenderableAasFile::prepare()
{
	if (!_aasFile) return;

	_normalShader = GlobalRenderSystem().capture("$AAS_AREA");

	constructRenderables();
}

void RenderableAasFile::constructRenderables()
{
	_renderableAabbs.clear();

	for (std::size_t areaNum = 0; areaNum < _aasFile->getNumAreas(); ++areaNum)
	{
		const IAasFile::Area& area = _aasFile->getArea(areaNum);

		_renderableAabbs.push_back(RenderableSolidAABB(area.bounds));
	}
}

} // namespace
