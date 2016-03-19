#pragma once

#include "imousetool.h"
#include "render/View.h"

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

public:
    ManipulateMouseTool(SelectionSystem& selectionSystem);

    const std::string& getName() override;
    const std::string& getDisplayName() override;

    Result onMouseDown(Event& ev) override;
    Result onMouseMove(Event& ev) override;
    Result onMouseUp(Event& ev) override;

    void onCancel(IInteractiveView& view) override;

    virtual unsigned int getPointerMode() override;
};

}
