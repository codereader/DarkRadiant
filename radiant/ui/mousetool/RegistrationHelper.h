#pragma once

#include "imousetool.h"
#include "selection/SelectionMouseTools.h"
#include "selection/ManipulateMouseTool.h"

namespace ui
{

class MouseToolRegistrationHelper
{
public:
	static void RegisterTools()
	{
        // Orthoview: manipulate and all the non-face selection tools
        IMouseToolGroup& orthoGroup = GlobalMouseToolManager().getGroup(IMouseToolGroup::Type::OrthoView);

        orthoGroup.registerMouseTool(std::make_shared<ManipulateMouseTool>());
        orthoGroup.registerMouseTool(std::make_shared<DragSelectionMouseTool>());
        orthoGroup.registerMouseTool(std::make_shared<CycleSelectionMouseTool>());

        // Camera: manipulation plus all selection tools, including the face-only tools
        IMouseToolGroup& camGroup = GlobalMouseToolManager().getGroup(IMouseToolGroup::Type::CameraView);

        camGroup.registerMouseTool(std::make_shared<ManipulateMouseTool>());
        camGroup.registerMouseTool(std::make_shared<DragSelectionMouseTool>());
        camGroup.registerMouseTool(std::make_shared<DragSelectionMouseToolFaceOnly>());
        camGroup.registerMouseTool(std::make_shared<CycleSelectionMouseTool>());
        camGroup.registerMouseTool(std::make_shared<CycleSelectionMouseToolFaceOnly>());
	}
};

}
