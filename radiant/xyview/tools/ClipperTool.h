#pragma once

#include "imousetool.h"


namespace ui
{

class XYMouseToolEvent;

/**
 * The Clipper tool operates on the XY view and allows the user
 * to place new and move existing clip points.
 */
class ClipperTool :
    public MouseTool
{
public:
    const std::string& getName();

    Result onMouseDown(Event& ev);
    Result onMouseMove(Event& ev);
    Result onMouseUp(Event& ev);

    bool alwaysReceivesMoveEvents();

private:
    void dropClipPoint(XYMouseToolEvent& event);
};

} // namespace
