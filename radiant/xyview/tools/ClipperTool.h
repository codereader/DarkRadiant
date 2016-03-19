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
    const std::string& getName() override;
    const std::string& getDisplayName() override;

    Result onMouseDown(Event& ev) override;
    Result onMouseMove(Event& ev) override;
    Result onMouseUp(Event& ev) override;

    bool alwaysReceivesMoveEvents() override;

private:
    void dropClipPoint(XYMouseToolEvent& event);
};

} // namespace
