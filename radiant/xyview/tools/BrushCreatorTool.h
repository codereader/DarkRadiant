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
    const std::string& getName();

    Result onMouseDown(Event& ev);
    Result onMouseMove(Event& ev);
    Result onMouseUp(Event& ev);

    void onCancel();
};

} // namespace
