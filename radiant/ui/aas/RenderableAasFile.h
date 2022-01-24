#pragma once

#include <list>
#include <sigc++/trackable.h>

#include "irenderable.h"
#include "irender.h"
#include "iaasfile.h"

#include "entitylib.h"
#include "render/RenderableBoundingBoxes.h"
#include "render/RenderableText.h"

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

    std::list<RenderableSolidAABB> _renderableAabbs;
    std::vector<AABB> _areas;
    std::vector<AABB> _visibleAreas;

	bool _renderNumbers;
	bool _hideDistantAreas;
	float _hideDistanceSquared;

    render::RenderableBoundingBoxes _renderableAreas;
    std::map<int, render::RenderableText> _renderableNumbers;

public:
	RenderableAasFile();

   void clear();

    void setRenderSystem(const RenderSystemPtr& renderSystem) override {}
    void onPreRender(const VolumeTest& volume) override;
	void renderSolid(IRenderableCollector& collector, const VolumeTest& volume) const override;
	void renderWireframe(IRenderableCollector& collector, const VolumeTest& volume) const override;
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
