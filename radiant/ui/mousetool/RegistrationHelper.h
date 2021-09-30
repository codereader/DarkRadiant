#pragma once

#include "imousetool.h"
#include "selection/SelectionMouseTools.h"
#include "selection/SceneManipulateMouseTool.h"
#include "xyview/tools/MoveViewTool.h"
#include "xyview/tools/ZoomTool.h"
#include "textool/tools/TextureToolSelectionTool.h"
#include "textool/tools/TextureToolCycleSelectionTool.h"
#include "textool/tools/TextureToolManipulateMouseTool.h"
#include "textool/TexTool.h"

namespace ui
{

class MouseToolRegistrationHelper
{
public:
	static void RegisterTools()
	{
        // Orthoview: manipulate and all the non-face selection tools
        auto& orthoGroup = GlobalMouseToolManager().getGroup(IMouseToolGroup::Type::OrthoView);

        orthoGroup.registerMouseTool(std::make_shared<SceneManipulateMouseTool>());
        orthoGroup.registerMouseTool(std::make_shared<BasicSelectionTool>());
        orthoGroup.registerMouseTool(std::make_shared<CycleSelectionMouseTool>());

        // Camera: manipulation plus all selection tools, including the face-only tools
        auto& camGroup = GlobalMouseToolManager().getGroup(IMouseToolGroup::Type::CameraView);

        camGroup.registerMouseTool(std::make_shared<SceneManipulateMouseTool>());
        camGroup.registerMouseTool(std::make_shared<BasicSelectionTool>());
        camGroup.registerMouseTool(std::make_shared<DragSelectionMouseToolFaceOnly>());
        camGroup.registerMouseTool(std::make_shared<CycleSelectionMouseTool>());
        camGroup.registerMouseTool(std::make_shared<CycleSelectionMouseToolFaceOnly>());

        auto& texToolGroup = GlobalMouseToolManager().getGroup(IMouseToolGroup::Type::TextureTool);

        texToolGroup.registerMouseTool(std::make_shared<MoveViewTool>());
        texToolGroup.registerMouseTool(std::make_shared<ZoomTool>());
        texToolGroup.registerMouseTool(std::make_shared<TextureToolSelectionTool>());
        texToolGroup.registerMouseTool(std::make_shared<TextureToolCycleSelectionTool>());
        texToolGroup.registerMouseTool(std::make_shared<TextureToolManipulateMouseTool>());
	}
};

}
