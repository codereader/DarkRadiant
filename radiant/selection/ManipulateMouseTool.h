#pragma once

#include "irender.h"
#include "imousetool.h"
#include "render/View.h"
#include "math/Vector2.h"

class SelectionSystem;

namespace ui
{

/**
 * greebo: This is the tool handling the manipulation mouse operations, it basically just
 * passes all the mouse clicks back to the SelectionSystem, trying to select something 
 * that can be manipulated (patches, lights, drag-resizable objects, vertices,...)
 */
class ManipulateMouseTool :
    public MouseTool
{
private:
    float _selectEpsilon;

    render::View _view;

    SelectionSystem& _selectionSystem;

	Matrix4 _pivot2worldStart;
	bool _manipulationActive;

	Vector2 _deviceStart;
	bool _undoBegun;

#ifdef _DEBUG
	std::string _debugText;
#endif

	ShaderPtr _pointShader;

public:
    ManipulateMouseTool(SelectionSystem& selectionSystem);

    const std::string& getName() override;
    const std::string& getDisplayName() override;

    Result onMouseDown(Event& ev) override;
    Result onMouseMove(Event& ev) override;
    Result onMouseUp(Event& ev) override;

    void onMouseCaptureLost(IInteractiveView& view) override;
    Result onCancel(IInteractiveView& view) override;

    virtual unsigned int getPointerMode() override;
    virtual unsigned int getRefreshMode() override;

	void renderOverlay() override;
	void render(RenderSystem& renderSystem, RenderableCollector& collector, const VolumeTest& volume) override;

private:
	bool selectManipulator(const render::View& view, const Vector2& devicePoint, const Vector2& deviceEpsilon);
	void handleMouseMove(const render::View& view, const Vector2& devicePoint);
	void freezeTransforms();
	void endMove();
	void cancelMove();
	bool nothingSelected() const;
};

}
