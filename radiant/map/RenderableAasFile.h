#pragma once

#include <list>
#include <sigc++/trackable.h>

#include "irenderable.h"
#include "irender.h"
#include "iaasfile.h"

#include "entitylib.h"

namespace map
{

const char* const RKEY_SHOW_AAS_AREA_NUMBERS = "user/ui/aasViewer/showNumbers";

// Renderable drawing all the area bounds of the attached AAS file,
// optionally showing the area numbers too
class RenderableAasFile :
    public Renderable,
	public OpenGLRenderable,
	public sigc::trackable
{
private:
    RenderSystemPtr _renderSystem;

    IAasFilePtr _aasFile;

	ShaderPtr _normalShader;

    std::list<RenderableSolidAABB> _renderableAabbs;

	bool _renderNumbers;

public:
	RenderableAasFile();

	void setRenderSystem(const RenderSystemPtr& renderSystem) override;
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override;
	bool isHighlighted() const override;

	void setAasFile(const IAasFilePtr& aasFile);

	void render(const RenderInfo& info) const override;

private:
	void prepare();
	void constructRenderables();
};

} // namespace
