#pragma once

#include "irender.h"
#include "imousetool.h"
#include "render/View.h"
#include "math/Vector2.h"
#include "selection/ManipulateMouseTool.h"

namespace ui
{

class TexTool;

/**
 * Specialised manipulation operations for the texture tool
 */
class TextureToolManipulateMouseTool :
    public ManipulateMouseTool
{
private:
    float _selectEpsilon;

    render::View _view;

	Matrix4 _pivot2worldStart;
	bool _manipulationActive;

	Vector2 _deviceStart;
	bool _undoBegun;

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

protected:
    virtual selection::IManipulator::Ptr getActiveManipulator() override;

private:
	bool selectManipulator(const render::View& view, const Vector2& devicePoint, const Vector2& deviceEpsilon);
	void handleMouseMove(const render::View& view, const Vector2& devicePoint);
	void freezeTransforms();
	void endMove();
	void cancelMove();
	bool nothingSelected() const;
};

}
