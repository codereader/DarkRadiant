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

    auto renderSystem = GlobalMapModule().getRoot()->getRenderSystem();

    if (!renderSystem)
    {
        return;
    }

    if (!_normalShader)
    {
        _normalShader = renderSystem->capture(BuiltInShaderType::AasAreaBounds);
    }

    if (!_textRenderer)
    {
        _textRenderer = renderSystem->captureTextRenderer(IGLFont::Style::Sans, 14);
    }

    if (_hideDistantAreas)
    {
        // Get the camera position for distance clipping
        auto invModelView = volume.GetModelview().getFullInverse();
        auto viewPos = invModelView.tCol().getProjected();

        _visibleAreas.clear();

        for (const auto& area : _areas)
        {
            if ((area.getOrigin() - viewPos).getLengthSquared() > _hideDistanceSquared)
            {
                continue;
            }

            _visibleAreas.push_back(area);
        }

        for (auto& [areaNum, text] : _renderableNumbers)
        {
            text.setVisible(_renderNumbers && 
                (_aasFile->getArea(areaNum).center - viewPos).getLengthSquared() <= _hideDistanceSquared);
            text.update(_textRenderer);
        }

        _renderableAreas.queueUpdate();
    }
    else
    {
        for (auto& [_, text] : _renderableNumbers)
        {
            text.setVisible(_renderNumbers);
            text.update(_textRenderer);
        }
    }

    _renderableAreas.update(_normalShader);
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
    _areas.clear();
    _renderableNumbers.clear();

	for (std::size_t areaNum = 0; areaNum < _aasFile->getNumAreas(); ++areaNum)
	{
		const IAasFile::Area& area = _aasFile->getArea(static_cast<int>(areaNum));

		_areas.push_back(area.bounds);

        // Allocate a new RenderableNumber for each area
        _renderableNumbers.try_emplace(areaNum, string::to_string(areaNum), area.center, Vector4(1, 1, 1, 1));
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
    _renderableNumbers.clear();
    _normalShader.reset();
    _textRenderer.reset();
}

} // namespace
