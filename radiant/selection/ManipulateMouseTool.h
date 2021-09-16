#pragma once

#include "irender.h"
#include "imousetool.h"
#include "render/View.h"
#include "math/Vector2.h"
#include "math/Matrix4.h"

namespace ui
{

/**
 * greebo: This is the base tool handling the manipulation mouse operations, it performs
 * the selection tests, is handling mouse movements, captures, emits manipulation events, etc.
 * 
 * It relies on subclasses implementing a number of pure virtuals. There exist specialised variants
 * of this tool which are designed to work with the GlobalSelectionSystem and the Texture Tool.
 */
class ManipulateMouseTool :
    public MouseTool
{
private:
    float _selectEpsilon;

protected:
	Matrix4 _pivot2worldStart;
	bool _manipulationActive;

	Vector2 _deviceStart;
	bool _undoBegun;

#ifdef _DEBUG
	std::string _debugText;
#endif

public:
    ManipulateMouseTool();
    virtual ~ManipulateMouseTool() {}

    virtual Result onMouseDown(Event& ev) override;
    virtual Result onMouseMove(Event& ev) override;
    virtual Result onMouseUp(Event& ev) override;

    virtual void onMouseCaptureLost(IInteractiveView& view) override;
    virtual Result onCancel(IInteractiveView& view) override;

    virtual unsigned int getPointerMode() override;
    virtual unsigned int getRefreshMode() override;

	virtual void renderOverlay() override;

protected:
    virtual selection::IManipulator::Ptr getActiveManipulator() = 0;
	virtual bool selectManipulator(const render::View& view, const Vector2& devicePoint, const Vector2& deviceEpsilon);

    virtual void onManipulationStart() = 0;
    virtual void onManipulationChanged() = 0;
    virtual void onManipulationCancelled() = 0;
	virtual void onManipulationFinished() = 0;

    virtual bool manipulationIsPossible() = 0;
    virtual Matrix4 getPivot2World() = 0;

private:
	void handleMouseMove(const render::View& view, const Vector2& devicePoint);
	void endMove();
	void cancelMove();
};

}
