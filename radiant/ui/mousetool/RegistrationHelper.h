#pragma once

#include "imousetool.h"
#include "selection/SelectionMouseTools.h"
#include "selection/ManipulateMouseTool.h"
#include "xyview/tools/MoveViewTool.h"

namespace ui
{

class MouseToolRegistrationHelper
{
public:
	static void RegisterTools()
	{
        // Orthoview: manipulate and all the non-face selection tools
        auto& orthoGroup = GlobalMouseToolManager().getGroup(IMouseToolGroup::Type::OrthoView);

        orthoGroup.registerMouseTool(std::make_shared<ManipulateMouseTool>());
        orthoGroup.registerMouseTool(std::make_shared<BasicSelectionTool>());
        orthoGroup.registerMouseTool(std::make_shared<CycleSelectionMouseTool>());

        // Camera: manipulation plus all selection tools, including the face-only tools
        auto& camGroup = GlobalMouseToolManager().getGroup(IMouseToolGroup::Type::CameraView);

        camGroup.registerMouseTool(std::make_shared<ManipulateMouseTool>());
        camGroup.registerMouseTool(std::make_shared<BasicSelectionTool>());
        camGroup.registerMouseTool(std::make_shared<DragSelectionMouseToolFaceOnly>());
        camGroup.registerMouseTool(std::make_shared<CycleSelectionMouseTool>());
        camGroup.registerMouseTool(std::make_shared<CycleSelectionMouseToolFaceOnly>());

        auto& texToolGroup = GlobalMouseToolManager().getGroup(IMouseToolGroup::Type::TextureTool);

        texToolGroup.registerMouseTool(std::make_shared<MoveViewTool>());
	}
};

}
