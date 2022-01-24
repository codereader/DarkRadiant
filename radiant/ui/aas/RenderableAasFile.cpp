#include "RenderableAasFile.h"

#include "iregistry.h"
#include "ui/imainframe.h"
#include "ivolumetest.h"

#include "registry/registry.h"

namespace map
{

RenderableAasFile::RenderableAasFile() :
	_renderNumbers(registry::getValue<bool>(RKEY_SHOW_AAS_AREA_NUMBERS)),
	_hideDistantAreas(registry::getValue<bool>(RKEY_HIDE_DISTANT_AAS_AREAS)),
	_hideDistanceSquared(registry::getValue<float>(RKEY_AAS_AREA_HIDE_DISTANCE)),
    _renderableAreas(_visibleAreas, { 1,1,1,1 })
{
	_hideDistanceSquared *= _hideDistanceSquared;

	GlobalRegistry().signalForKey(RKEY_SHOW_AAS_AREA_NUMBERS).connect(
        sigc::mem_fun(*this, &RenderableAasFile::onShowAreaNumbersChanged));

	GlobalRegistry().signalForKey(RKEY_HIDE_DISTANT_AAS_AREAS).connect(
        sigc::mem_fun(*this, &RenderableAasFile::onHideDistantAreasChanged));
}

void RenderableAasFile::onShowAreaNumbersChanged()
{
    _renderNumbers = registry::getValue<bool>(RKEY_SHOW_AAS_AREA_NUMBERS);
    GlobalMainFrame().updateAllWindows();
}

void RenderableAasFile::onHideDistantAreasChanged()
{
    _hideDistantAreas = registry::getValue<bool>(RKEY_HIDE_DISTANT_AAS_AREAS);
    _hideDistanceSquared = registry::getValue<float>(RKEY_AAS_AREA_HIDE_DISTANCE);
    _hideDistanceSquared *= _hideDistanceSquared;

    if (!_hideDistantAreas)
    {
        _visibleAreas = _areas;
    }

    _renderableAreas.queueUpdate();
    GlobalMainFrame().updateAllWindows();
}

void RenderableAasFile::onPreRender(const VolumeTest& volume)
{
    if (!_aasFile || !volume.fill()) return; // only react to camera views

    if (_hideDistantAreas)
    {
        // Get the camera position for distance clipping
        auto invModelView = volume.GetModelview().getFullInverse();
        auto viewPos = invModelView.tCol().getProjected();

        _visibleAreas.clear();

        for (const auto& area : _areas)
        {
            if (_hideDistantAreas && (area.getOrigin() - viewPos).getLengthSquared() > _hideDistanceSquared)
            {
                continue;
            }

            _visibleAreas.push_back(area);
        }

        _renderableAreas.queueUpdate();
    }

    auto renderSystem = GlobalMapModule().getRoot()->getRenderSystem();

    if (!renderSystem)
    {
        return;
    }

    if (!_normalShader)
    {
        _normalShader = renderSystem->capture("$AAS_AREA");
    }

    _renderableAreas.update(_normalShader);
}

void RenderableAasFile::renderSolid(IRenderableCollector& collector, const VolumeTest& volume) const
{
#if 0
	if (!_aasFile) return;

	// Get the camera position for distance clipping
	Matrix4 invModelView = volume.GetModelview().getFullInverse();
	Vector3 viewPos = invModelView.tCol().getProjected();

	for (const RenderableSolidAABB& aabb : _renderableAabbs)
	{
		if (_hideDistantAreas && (aabb.getAABB().getOrigin() - viewPos).getLengthSquared() > _hideDistanceSquared)
		{
			continue;
		}

		collector.addRenderable(*_normalShader, aabb, Matrix4::getIdentity());
	}
#endif
	if (_renderNumbers)
	{
		collector.addRenderable(*_normalShader, *this, Matrix4::getIdentity());
	}
}

void RenderableAasFile::renderWireframe(IRenderableCollector& collector, const VolumeTest& volume) const
{
	// Do nothing in wireframe mode
}

std::size_t RenderableAasFile::getHighlightFlags()
{
	return Highlight::NoHighlight;
}

void RenderableAasFile::setAasFile(const IAasFilePtr& aasFile)
{
    clear();

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
    if (!_aasFile)
    {
        clear();
        return;
    }

	constructRenderables();
}

void RenderableAasFile::constructRenderables()
{
	_renderableAabbs.clear();
    _areas.clear();

	for (std::size_t areaNum = 0; areaNum < _aasFile->getNumAreas(); ++areaNum)
	{
		const IAasFile::Area& area = _aasFile->getArea(static_cast<int>(areaNum));

		_renderableAabbs.push_back(RenderableSolidAABB(area.bounds));
		_areas.push_back(area.bounds);
	}

    if (!_hideDistantAreas)
    {
        _visibleAreas = _areas;
    }

    _renderableAreas.queueUpdate();
}

void RenderableAasFile::clear()
{
    _aasFile.reset();
    _renderableAreas.clear();
    _areas.clear();
    _visibleAreas.clear();
    _normalShader.reset();
}

} // namespace
