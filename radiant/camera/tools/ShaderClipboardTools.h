#pragma once

#include "imousetool.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "selection/algorithm/Shader.h"
#include <functional>

namespace ui
{

class ShaderMouseToolBase :
    public MouseTool
{
private:
    const std::string _toolName;

    std::function<void(SelectionTest&)> _action;

protected:
    ShaderMouseToolBase(const std::string& toolName, 
                        const std::function<void(SelectionTest&)>& action) :
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
                SelectionTestPtr selectionTest = camEvent.getView().createSelectionTestForPoint(camEvent.getDevicePosition());

                _action(*selectionTest);
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
                SelectionTestPtr selectionTest = camEvent.getView().createSelectionTestForPoint(camEvent.getDevicePosition());

                _action(*selectionTest);
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
        return Result::Finished;
    }
};

// Copies the shader of the clicked object to the clipboard
class PickShaderTool :
    public ShaderMouseToolBase
{
public:
    PickShaderTool() :
        ShaderMouseToolBase("PickShaderTool", 
            std::bind(&PickShaderTool::onAction, this, std::placeholders::_1))
    {}

private:
    void onAction(SelectionTest& selectionTest)
    {
        // Set the source texturable from the given test
        GlobalShaderClipboard().setSource(selectionTest);
    }
};

// Pastes the shader from the clipboard to the clicked object
class PasteShaderProjectedTool :
    public ShaderMouseToolBase
{
public:
    PasteShaderProjectedTool() :
        ShaderMouseToolBase("PasteShaderProjectedTool",
            std::bind(&PasteShaderProjectedTool::onAction, this, std::placeholders::_1))
    {}

private:
    void onAction(SelectionTest& selectionTest)
    {
        // Paste the shader projected (TRUE), but not to an entire brush (FALSE)
        selection::algorithm::pasteShader(selectionTest, true, false);
    }
};

// Pastes the shader from the clipboard to the clicked object
class PasteShaderNaturalTool :
    public ShaderMouseToolBase
{
public:
    PasteShaderNaturalTool() :
        ShaderMouseToolBase("PasteShaderNaturalTool", 
            std::bind(&PasteShaderNaturalTool::onAction, this, std::placeholders::_1))
    {}

private:
    void onAction(SelectionTest& selectionTest)
    {
        // Paste the shader naturally (FALSE), but not to an entire brush (FALSE)
        selection::algorithm::pasteShader(selectionTest, false, false);
    }
};

// Pastes the texture coordinates from the clipboard's patch to the clicked patch
class PasteShaderCoordsTool :
    public ShaderMouseToolBase
{
public:
    PasteShaderCoordsTool() :
        ShaderMouseToolBase("PasteShaderCoordsTool", 
            std::bind(&PasteShaderCoordsTool::onAction, this, std::placeholders::_1))
    {}

private:
    void onAction(SelectionTest& selectionTest)
    {
        // Clone the texture coordinates from the patch in the clipboard
        selection::algorithm::pasteTextureCoords(selectionTest);
    }
};

// Pastes the shader from the clipboard to the entire brush
class PasteShaderToBrushTool :
    public ShaderMouseToolBase
{
public:
    PasteShaderToBrushTool() :
        ShaderMouseToolBase("PasteShaderToBrushTool",
            std::bind(&PasteShaderToBrushTool::onAction, this, std::placeholders::_1))
    {}

private:
    void onAction(SelectionTest& selectionTest)
    {
        // Paste the shader projected (TRUE), and to the entire brush (TRUE)
        selection::algorithm::pasteShader(selectionTest, true, true);
    }
};

// Pastes the shader name only from the clipboard
class PasteShaderNameTool :
    public ShaderMouseToolBase
{
public:
    PasteShaderNameTool() :
        ShaderMouseToolBase("PasteShaderNameTool",
            std::bind(&PasteShaderNameTool::onAction, this, std::placeholders::_1))
    {}

private:
    void onAction(SelectionTest& selectionTest)
    {
        // Paste the shader name only
        selection::algorithm::pasteShaderName(selectionTest);
    }
};

}
