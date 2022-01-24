#pragma once

#include <map>
#include <sigc++/trackable.h>

#include "irenderable.h"
#include "irender.h"
#include "iaasfile.h"

#include "render/RenderableBoundingBoxes.h"
#include "render/StaticRenderableText.h"

namespace map
{

const char* const RKEY_SHOW_AAS_AREA_NUMBERS = "user/ui/aasViewer/showNumbers";
const char* const RKEY_HIDE_DISTANT_AAS_AREAS = "user/ui/aasViewer/hideDistantAreas";
const char* const RKEY_AAS_AREA_HIDE_DISTANCE = "user/ui/aasViewer/hideDistance";

// Renderable drawing all the area bounds of the attached AAS file,
// optionally showing the area numbers too
class RenderableAasFile :
    public Renderable,
	public sigc::trackable
{
private:
    IAasFilePtr _aasFile;

	ShaderPtr _normalShader;
    ITextRenderer::Ptr _textRenderer;

    std::vector<AABB> _areas;
    std::vector<AABB> _visibleAreas;

	bool _renderNumbers;
	bool _hideDistantAreas;
	float _hideDistanceSquared;

    render::RenderableBoundingBoxes _renderableAreas;
    std::map<std::size_t, render::StaticRenderableText> _renderableNumbers;

public:
	RenderableAasFile();

   void clear();

    void setRenderSystem(const RenderSystemPtr& renderSystem) override {}
    void onPreRender(const VolumeTest& volume) override;
    void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override
    {}

	std::size_t getHighlightFlags() override;

	void setAasFile(const IAasFilePtr& aasFile);

private:
	void prepare();
	void constructRenderables();
    void onHideDistantAreasChanged();
    void onShowAreaNumbersChanged();
};

} // namespace
