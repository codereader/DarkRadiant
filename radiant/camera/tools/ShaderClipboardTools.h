#pragma once

#include "imousetool.h"
#include "i18n.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "selection/algorithm/Shader.h"
#include "CameraMouseToolEvent.h"
#include <functional>
#include "wxutil/dialog/MessageBox.h"

namespace ui
{

class ShaderMouseToolBase :
    public MouseTool
{
private:
    const std::string _toolName;
    const std::string _toolDisplayName;

    std::function<void(SelectionTest&)> _action;

protected:
    ShaderMouseToolBase(const std::string& toolName, const std::string& displayName,
                        const std::function<void(SelectionTest&)>& action) :
        _toolName(toolName),
        _toolDisplayName(displayName),
        _action(action)
    {}

    const std::string& getName() override
    {
        return _toolName;
    }

    const std::string& getDisplayName() override
    {
        return _toolDisplayName;
    }

    Result onMouseDown(Event& ev) override
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
        catch (selection::algorithm::InvalidOperationException& ex)
        {
            // Some operations fail and want to notify the user
            // we do this here to not steal the focus from our window
            // and indicate taht the tool state has been cancelled
            wxutil::Messagebox::ShowError(ex.what());

            return Result::Finished;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored; // not handled

    }

    Result onMouseMove(Event& ev) override
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
        catch (selection::algorithm::InvalidOperationException& ex)
        {
            // Some operations fail and want to notify the user
            // we do this here to not steal the focus from our window
            // and indicate taht the tool state has been cancelled
            wxutil::Messagebox::ShowError(ex.what());

            return Result::Finished;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored;
    }

    Result onMouseUp(Event& ev) override
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
        ShaderMouseToolBase(NAME(), _("Pick Shader"),
            std::bind(&PickShaderTool::onAction, this, std::placeholders::_1))
    {}

    static std::string NAME()
    {
        return "PickShaderTool";
    }

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
        ShaderMouseToolBase("PasteShaderProjectedTool", _("Paste Shader Projected"),
            std::bind(&PasteShaderProjectedTool::onAction, this, std::placeholders::_1))
    {}

private:
    void onAction(SelectionTest& selectionTest)
    {
        // Paste the shader projected (TRUE), but not to an entire brush (FALSE)
        // The InvalidOperationExceptions are handled by the calling code in the base class
        selection::algorithm::pasteShader(selectionTest, true, false);
    }
};

// Pastes the shader from the clipboard to the clicked object
class PasteShaderNaturalTool :
    public ShaderMouseToolBase
{
public:
    PasteShaderNaturalTool() :
        ShaderMouseToolBase("PasteShaderNaturalTool", _("Paste Shader Natural"),
            std::bind(&PasteShaderNaturalTool::onAction, this, std::placeholders::_1))
    {}

private:
    void onAction(SelectionTest& selectionTest)
    {
        // Paste the shader naturally (FALSE), but not to an entire brush (FALSE)
        // The InvalidOperationExceptions are handled by the calling code in the base class
        selection::algorithm::pasteShader(selectionTest, false, false);
    }
};

// Pastes the texture coordinates from the clipboard's patch to the clicked patch
class PasteShaderCoordsTool :
    public ShaderMouseToolBase
{
public:
    PasteShaderCoordsTool() :
        ShaderMouseToolBase("PasteShaderCoordsTool", _("Paste Texture Coordinates"),
            std::bind(&PasteShaderCoordsTool::onAction, this, std::placeholders::_1))
    {}

private:
    void onAction(SelectionTest& selectionTest)
    {
        // Clone the texture coordinates from the patch in the clipboard
        // The InvalidOperationExceptions are handled by the calling code in the base class
        selection::algorithm::pasteTextureCoords(selectionTest);
    }
};

// Pastes the shader from the clipboard to the entire brush
class PasteShaderToBrushTool :
    public ShaderMouseToolBase
{
public:
    PasteShaderToBrushTool() :
        ShaderMouseToolBase("PasteShaderToBrushTool", _("Paste Shader to all Brush Faces"),
            std::bind(&PasteShaderToBrushTool::onAction, this, std::placeholders::_1))
    {}

private:
    void onAction(SelectionTest& selectionTest)
    {
        // Paste the shader projected (TRUE), and to the entire brush (TRUE)
        // The InvalidOperationExceptions are handled by the calling code in the base class
        selection::algorithm::pasteShader(selectionTest, true, true);
    }
};

// Pastes the shader name only from the clipboard
class PasteShaderNameTool :
    public ShaderMouseToolBase
{
public:
    PasteShaderNameTool() :
        ShaderMouseToolBase("PasteShaderNameTool", _("Paste Shader Name"),
            std::bind(&PasteShaderNameTool::onAction, this, std::placeholders::_1))
    {}

private:
    void onAction(SelectionTest& selectionTest)
    {
        // Paste the shader name only
        // The InvalidOperationExceptions are handled by the calling code in the base class
        selection::algorithm::pasteShaderName(selectionTest);
    }
};

}
