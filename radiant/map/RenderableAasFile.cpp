#include "RenderableAasFile.h"

#include "iregistry.h"
#include "imainframe.h"
#include "ivolumetest.h"

#include "registry/registry.h"

namespace map
{

RenderableAasFile::RenderableAasFile() :
	_renderNumbers(registry::getValue<bool>(RKEY_SHOW_AAS_AREA_NUMBERS)),
	_hideDistantAreas(registry::getValue<bool>(RKEY_HIDE_DISTANT_AAS_AREAS)),
	_hideDistanceSquared(registry::getValue<float>(RKEY_AAS_AREA_HIDE_DISTANCE))
{
	_hideDistanceSquared *= _hideDistanceSquared;

	GlobalRegistry().signalForKey(RKEY_SHOW_AAS_AREA_NUMBERS).connect([this]()
	{
		_renderNumbers = registry::getValue<bool>(RKEY_SHOW_AAS_AREA_NUMBERS);
		GlobalMainFrame().updateAllWindows();
	});

	GlobalRegistry().signalForKey(RKEY_HIDE_DISTANT_AAS_AREAS).connect([this]()
	{
		_hideDistantAreas = registry::getValue<bool>(RKEY_HIDE_DISTANT_AAS_AREAS);
		_hideDistanceSquared = registry::getValue<float>(RKEY_AAS_AREA_HIDE_DISTANCE);
		_hideDistanceSquared *= _hideDistanceSquared;
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

	// Get the camera position for distance clipping
	Matrix4 invModelView = volume.GetModelview().getFullInverse();
	Vector3 viewPos = invModelView.t().getProjected();

	for (const RenderableSolidAABB& aabb : _renderableAabbs)
	{
		if (_hideDistantAreas && (aabb.getAABB().getOrigin() - viewPos).getLengthSquared() > _hideDistanceSquared)
		{
			continue;
		}

		collector.addRenderable(_normalShader, aabb, Matrix4::getIdentity());
	}

	if (_renderNumbers)
	{
		collector.addRenderable(_normalShader, *this, Matrix4::getIdentity());
	}
}

void RenderableAasFile::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	// Do nothing in wireframe mode
}

std::size_t RenderableAasFile::getHighlightFlags()
{
	return Highlight::NoHighlight;
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
		const IAasFile::Area& area = _aasFile->getArea(static_cast<int>(areaNum));

		if (_hideDistantAreas && (area.center - info.getViewerLocation()).getLengthSquared() > _hideDistanceSquared)
		{
			continue;
		}

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
		const IAasFile::Area& area = _aasFile->getArea(static_cast<int>(areaNum));

		_renderableAabbs.push_back(RenderableSolidAABB(area.bounds));
	}
}

} // namespace
