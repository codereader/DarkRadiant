#include "TextureToolManipulateMouseTool.h"

#include "i18n.h"
#include "itexturetoolmodel.h"
#include "textool/TexTool.h"

namespace ui
{

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
    return TexTool::Instance().getPivot2World();
}

void TextureToolManipulateMouseTool::onManipulationStart()
{
    TexTool::Instance().onManipulationStart();
}

void TextureToolManipulateMouseTool::onManipulationChanged()
{
    TexTool::Instance().onManipulationChanged();
}

void TextureToolManipulateMouseTool::onManipulationFinished()
{
    TexTool::Instance().onManipulationEnd();
}

void TextureToolManipulateMouseTool::onManipulationCancelled()
{
    TexTool::Instance().onManipulationCancelled();
}

}
