#pragma once

#include "imousetool.h"
#include "iorthoview.h"

namespace ui
{

/**
 * The ZoomTool implements the drag-to-zoom functionality used in
 * the orthoviews. The mouse pointer is captured during its active
 * phase and only the move deltas are processed to zoom in and out.
 */
class ZoomTool :
    public MouseTool
{
private:
    int _dragZoom;

public:
    const std::string& getName()
    {
        static std::string name("ZoomTool");
        return name;
    }

    unsigned int getPointerMode()
    {
        return PointerMode::Capture | PointerMode::Freeze | PointerMode::Hidden;
    }

    Result onMouseDown(Event& ev)
    {
        try
        {
            dynamic_cast<XYMouseToolEvent&>(ev);

            _dragZoom = 0;

            return Result::Activated;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored; // not handled
    }

    Result onMouseMove(Event& ev)
    {
        try
        {
            // We use capture mode, so xy event will contain the delta only
            XYMouseToolEvent& xyEvent = dynamic_cast<XYMouseToolEvent&>(ev);

            if (xyEvent.getDeviceDelta().y() != 0)
            {
                _dragZoom += xyEvent.getDeviceDelta().y();

                while (abs(_dragZoom) > 8)
                {
                    if (_dragZoom > 0)
                    {
                        xyEvent.getView().zoomOut();
                        _dragZoom -= 8;
                    }
                    else
                    {
                        xyEvent.getView().zoomIn();
                        _dragZoom += 8;
                    }
                }
            }

            return Result::Continued;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored;
    }

    Result onMouseUp(Event& ev)
    {
        try
        {
            // We only operate on XY view events, so attempt to cast
            dynamic_cast<XYMouseToolEvent&>(ev);
            return Result::Finished;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored;
    }
};

}
