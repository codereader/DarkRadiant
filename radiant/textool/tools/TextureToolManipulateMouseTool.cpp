#include "TextureToolManipulateMouseTool.h"

#include "i18n.h"
#include "itexturetoolmodel.h"

namespace ui
{

TextureToolManipulateMouseTool::TextureToolManipulateMouseTool() :
    _gridEnabled(textool::RKEY_GRID_STATE)
{}

const std::string& TextureToolManipulateMouseTool::getName()
{
    static std::string name("TextureToolManipulateMouseTool");
    return name;
}

const std::string& TextureToolManipulateMouseTool::getDisplayName()
{
    static std::string displayName(_("Manipulate"));
    return displayName;
}

selection::IManipulator::Ptr TextureToolManipulateMouseTool::getActiveManipulator()
{
    return GlobalTextureToolSelectionSystem().getActiveManipulator();
}

bool TextureToolManipulateMouseTool::manipulationIsPossible()
{
    return true;
}

Matrix4 TextureToolManipulateMouseTool::getPivot2World()
{
    return GlobalTextureToolSelectionSystem().getPivot2World();
}

void TextureToolManipulateMouseTool::onManipulationStart()
{
    GlobalTextureToolSelectionSystem().onManipulationStart();
}

void TextureToolManipulateMouseTool::onManipulationChanged()
{
    GlobalTextureToolSelectionSystem().onManipulationChanged();
}

void TextureToolManipulateMouseTool::onManipulationFinished()
{
    GlobalTextureToolSelectionSystem().onManipulationFinished();
}

void TextureToolManipulateMouseTool::onManipulationCancelled()
{
    GlobalTextureToolSelectionSystem().onManipulationCancelled();
}

bool TextureToolManipulateMouseTool::gridIsEnabled()
{
    return _gridEnabled.get();
}

}
