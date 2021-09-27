#pragma once

#include "imodule.h"
#include "render/Colour4b.h"

namespace textool
{

enum class ColourScheme
{
    Light,
    Dark,
    Colourful,
};

enum class SchemeElement
{
    MinorGrid,
    MajorGrid,
};

class IColourSchemeManager :
    public RegisterableModule
{
public:
    virtual ~IColourSchemeManager() {}

    // Activates the given scheme
    virtual void setActiveScheme(ColourScheme scheme) = 0;

    virtual Colour4b getColour(SchemeElement element) = 0;
};

}

constexpr const char* const MODULE_TEXTOOL_COLOURSCHEME_MANAGER("TextureToolColourSchemeManager");

inline textool::IColourSchemeManager& GlobalTextureToolColourSchemeManager()
{
    static module::InstanceReference<textool::IColourSchemeManager> _reference(MODULE_TEXTOOL_COLOURSCHEME_MANAGER);
    return _reference;
}
