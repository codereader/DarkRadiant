#pragma once

#include "irender.h"
#include "imousetool.h"
#include "render/View.h"
#include "math/Vector2.h"

namespace ui
{

class TexTool;

/**
 * greebo: This is the tool handling the manipulation mouse operations for the texture tool.
 * TODO: It's similar to the ManipulateMouseTool, so it could #share a lot of code.
 */
class TextureToolManipulateMouseTool :
    public MouseTool
{
private:
    float _selectEpsilon;

    render::View _view;

	Matrix4 _pivot2worldStart;
	bool _manipulationActive;

	Vector2 _deviceStart;
	bool _undoBegun;

#ifdef _DEBUG
	std::string _debugText;
#endif

	ShaderPtr _pointShader;

public:
    TextureToolManipulateMouseTool();

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

private:
	bool selectManipulator(const render::View& view, const Vector2& devicePoint, const Vector2& deviceEpsilon);
	void handleMouseMove(const render::View& view, const Vector2& devicePoint);
	void freezeTransforms();
	void endMove();
	void cancelMove();
	bool nothingSelected() const;
};

}
