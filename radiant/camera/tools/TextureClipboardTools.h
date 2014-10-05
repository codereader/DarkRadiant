#pragma once

#include "imousetool.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "selection/algorithm/Shader.h"
#include <functional>

namespace ui
{

class SingleClickCameraMouseTool :
    public MouseTool
{
private:
    const std::string _toolName;

    std::function<void(CameraMouseToolEvent&)> _action;

protected:
    SingleClickCameraMouseTool(const std::string& toolName,
                               const std::function<void(CameraMouseToolEvent&)>& action) :
        _toolName(toolName),
        _action(action)
    {}

    const std::string& getName()
    {
        return _toolName;
    }

    Result onMouseDown(Event& ev)
    {
        try
        {
            CameraMouseToolEvent& camEvent = dynamic_cast<CameraMouseToolEvent&>(ev);

            if (_action)
            {
                _action(camEvent);
            }

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
            CameraMouseToolEvent& camEvent = dynamic_cast<CameraMouseToolEvent&>(ev);

            if (_action)
            {
                _action(camEvent);
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
            return Result::Finished;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored;
    }
};

// Copies the shader of the clicked object to the clipboard
class PickTextureTool :
    public SingleClickCameraMouseTool
{
public:
    PickTextureTool() :
        SingleClickCameraMouseTool("PickTextureTool", 
            std::bind(&PickTextureTool::onAction, this, std::placeholders::_1))
    {}

private:
    void onAction(CameraMouseToolEvent& camEvent)
    {
        SelectionTestPtr selectionTest = camEvent.getView().createSelectionTestForPoint(camEvent.getDevicePosition());

        // Set the source texturable from the given test
        GlobalShaderClipboard().setSource(*selectionTest);
    }
};

// Pastes the shader from the clipboard to the clicked object
class PasteTextureProjectedTool :
    public SingleClickCameraMouseTool
{
public:
    PasteTextureProjectedTool() :
        SingleClickCameraMouseTool("PasteTextureProjectedTool",
        std::bind(&PasteTextureProjectedTool::onAction, this, std::placeholders::_1))
    {}

private:
    void onAction(CameraMouseToolEvent& camEvent)
    {
        SelectionTestPtr selectionTest = camEvent.getView().createSelectionTestForPoint(camEvent.getDevicePosition());

        // Paste the shader projected (TRUE), but not to an entire brush (FALSE)
        selection::algorithm::pasteShader(*selectionTest, true, false);
    }
};

// Pastes the shader from the clipboard to the clicked object
class PasteTextureNaturalTool :
    public SingleClickCameraMouseTool
{
public:
    PasteTextureNaturalTool() :
        SingleClickCameraMouseTool("PasteTextureNaturalTool", 
            std::bind(&PasteTextureNaturalTool::onAction, this, std::placeholders::_1))
    {}

private:
    void onAction(CameraMouseToolEvent& camEvent)
    {
        SelectionTestPtr selectionTest = camEvent.getView().createSelectionTestForPoint(camEvent.getDevicePosition());

        // Paste the shader naturally (FALSE), but not to an entire brush (FALSE)
        selection::algorithm::pasteShader(*selectionTest, false, false);
    }
};

// Pastes the texture coordinates from the clipboard's patch to the clicked patch
class PasteTextureCoordsTool :
    public SingleClickCameraMouseTool
{
public:
    PasteTextureCoordsTool() :
        SingleClickCameraMouseTool("PasteTextureCoordsTool", 
        std::bind(&PasteTextureCoordsTool::onAction, this, std::placeholders::_1))
    {}

private:
    void onAction(CameraMouseToolEvent& camEvent)
    {
        SelectionTestPtr selectionTest = camEvent.getView().createSelectionTestForPoint(camEvent.getDevicePosition());

        // Clone the texture coordinates from the patch in the clipboard
        selection::algorithm::pasteTextureCoords(*selectionTest);
    }
};

}
