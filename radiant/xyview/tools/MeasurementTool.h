#pragma once

#include "imousetool.h"
#include "iorthoview.h"
#include "math/Vector3.h"
#include "render.h"
#include "render/Colour4.h"
#include "render/RenderableVertexArray.h"

namespace ui
{

/**
* This tool can be used to measure distances.
* It is working on the orthoviews only.
*/
class MeasurementTool :
	public MouseTool
{
private:
    std::vector<Vertex3f> _vertices;
    render::RenderablePoints _points;
	render::RenderableLine _line;

	ShaderPtr _pointShader;
	ShaderPtr _wireShader;

	Colour4 _colour;

public:
	MeasurementTool();

	const std::string& getName() override;
	const std::string& getDisplayName() override;

	Result onMouseDown(Event& ev) override;
	Result onMouseMove(Event& ev) override;
	Result onMouseUp(Event& ev) override;

	unsigned int getPointerMode() override;
	unsigned int getRefreshMode() override;
	Result onCancel(IInteractiveView& view) override;
	void onMouseCaptureLost(IInteractiveView& view) override;

	void render(RenderSystem& renderSystem, IRenderableCollector& collector, const VolumeTest& volume) override;

private:
	void ensureShaders(RenderSystem& renderSystem);
};

} // namespace
