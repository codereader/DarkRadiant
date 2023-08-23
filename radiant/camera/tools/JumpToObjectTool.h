#pragma once

#include "imousetool.h"
#include "iscenegraph.h"
#include "iorthoview.h"
#include "i18n.h"
#include "../CameraWndManager.h"
#include "ObjectFinder.h"

#include "math/AABB.h"

namespace ui
{

class JumpToObjectTool :
    public MouseTool
{
public:
    const std::string& getName() override
    {
        static std::string name("JumpToObjectTool");
        return name;
    }

    const std::string& getDisplayName() override
    {
        static std::string displayName(_("Jump to Object"));
        return displayName;
    }

    Result onMouseDown(Event& ev) override
    {
        try
        {
            auto& camEvent = dynamic_cast<CameraMouseToolEvent&>(ev);

            auto selectionTest = camEvent.getView().createSelectionTestForPoint(camEvent.getDevicePosition());

            // Find a suitable target node
            camera::ObjectFinder finder(*selectionTest);
            GlobalSceneGraph().root()->traverse(finder);

            if (finder.getNode())
            {
                // A node has been found, get the bounding box
                auto found = finder.getNode()->worldAABB();

                // Focus the view at the center of the found AABB
                // Set the camera and the views to the given point
                GlobalCameraManager().focusAllCameras(found.origin, camEvent.getView().getCameraAngles());
                GlobalOrthoViewManager().setOrigin(found.origin);
            }

            return Result::Finished;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored; // not handled
    }

    Result onMouseMove(Event& ev) override
    {
        return Result::Finished;
    }

    Result onMouseUp(Event& ev) override
    {
        return Result::Finished;
    }
};

} // namespace
