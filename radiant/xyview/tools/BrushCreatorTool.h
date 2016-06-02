#pragma once

#include "imousetool.h"
#include "iorthoview.h"
#include "inode.h"
#include "math/Vector3.h"

namespace ui
{

/**
 * This tool provides the drag-to-create-a-brush functionality.
 * It is working on the orthoviews only.
 */
class BrushCreatorTool :
    public MouseTool
{
private:
    scene::INodePtr _brush;
    Vector3 _startPos;

public:
    const std::string& getName() override;
    const std::string& getDisplayName() override;

    Result onMouseDown(Event& ev) override;
    Result onMouseMove(Event& ev) override;
    Result onMouseUp(Event& ev) override;

    unsigned int getPointerMode() override;
    unsigned int getRefreshMode() override;
    Result onCancel(IInteractiveView& view) override;
    void onMouseCaptureLost(IInteractiveView& view) override;
};

} // namespace
