#pragma once

#include "imodule.h"
#include "render/Colour4.h"

namespace textool
{

enum class ColourScheme
{
    Light,
    Dark,
};

enum class SchemeElement
{
    GridText,
    MinorGrid,
    MajorGrid,
    SurfaceInSurfaceMode,
    SurfaceInComponentMode,
    Vertex,
    SelectedSurface,
    SelectedVertex,
    Manipulator,
    SelectedManipulator,
    ManipulatorSurface,
};

class IColourSchemeManager :
    public RegisterableModule
{
public:
    virtual ~IColourSchemeManager() {}

    // Activates the given scheme
    virtual void setActiveScheme(ColourScheme scheme) = 0;

    virtual Colour4 getColour(SchemeElement element) = 0;
};

}

constexpr const char* const MODULE_TEXTOOL_COLOURSCHEME_MANAGER("TextureToolColourSchemeManager");

inline textool::IColourSchemeManager& GlobalTextureToolColourSchemeManager()
{
    static module::InstanceReference<textool::IColourSchemeManager> _reference(MODULE_TEXTOOL_COLOURSCHEME_MANAGER);
    return _reference;
}
