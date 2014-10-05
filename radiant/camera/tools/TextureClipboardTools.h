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
    std::function<void(CameraMouseToolEvent&)> _mouseDownFunctor;

protected:
    SingleClickCameraMouseTool(const std::function<void(CameraMouseToolEvent&)>& mouseDownFunctor) :
        _mouseDownFunctor(mouseDownFunctor)
    {}

    Result onMouseDown(Event& ev)
    {
        try
        {
            CameraMouseToolEvent& camEvent = dynamic_cast<CameraMouseToolEvent&>(ev);

            if (_mouseDownFunctor)
            {
                _mouseDownFunctor(camEvent);
            }

            return Result::Finished;
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
            return Result::Ignored;
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
        SingleClickCameraMouseTool(std::bind(&PickTextureTool::onClick, this, std::placeholders::_1))
    {}

    const std::string& getName()
    {
        static std::string name("PickTextureTool");
        return name;
    }
    
private:
    void onClick(CameraMouseToolEvent& camEvent)
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
        SingleClickCameraMouseTool(std::bind(&PasteTextureProjectedTool::onClick, this, std::placeholders::_1))
    {}

    const std::string& getName()
    {
        static std::string name("PasteTextureProjectedTool");
        return name;
    }

private:
    void onClick(CameraMouseToolEvent& camEvent)
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
        SingleClickCameraMouseTool(std::bind(&PasteTextureNaturalTool::onClick, this, std::placeholders::_1))
    {}

    const std::string& getName()
    {
        static std::string name("PasteTextureNaturalTool");
        return name;
    }

private:
    void onClick(CameraMouseToolEvent& camEvent)
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
        SingleClickCameraMouseTool(std::bind(&PasteTextureCoordsTool::onClick, this, std::placeholders::_1))
    {}

    const std::string& getName()
    {
        static std::string name("PasteTextureCoordsTool");
        return name;
    }

private:
    void onClick(CameraMouseToolEvent& camEvent)
    {
        SelectionTestPtr selectionTest = camEvent.getView().createSelectionTestForPoint(camEvent.getDevicePosition());

        // Clone the texture coordinates from the patch in the clipboard
        selection::algorithm::pasteTextureCoords(*selectionTest);
    }
};

}
