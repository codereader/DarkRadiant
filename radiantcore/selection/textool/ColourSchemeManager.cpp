#include "itexturetoolcolours.h"

#include <map>
#include "module/StaticModule.h"

namespace textool
{

class ColourSchemeManager final :
    public IColourSchemeManager
{
private:
    using SchemeMap = std::map<SchemeElement, Colour4>;
    std::map<ColourScheme, SchemeMap> _schemes;

    ColourScheme _activeScheme;

public:
    const std::string& getName() const override
    {
        static std::string _name(MODULE_TEXTOOL_COLOURSCHEME_MANAGER);
        return _name;
    }

    const StringSet& getDependencies() const override
    {
        static StringSet _dependencies;
        return _dependencies;
    }

    void initialiseModule(const IApplicationContext& ctx) override
    {
        _activeScheme = ColourScheme::Dark;

        // Fill in the default scheme values
        _schemes[ColourScheme::Dark] = SchemeMap
        {
            { SchemeElement::GridText, { 0, 0, 0, 0.8f } },
            { SchemeElement::MajorGrid, { 0, 0, 0, 0.8f } },
            { SchemeElement::MinorGrid, { 0.2f, 0.2f, 0.2f, 0.4f} },
            { SchemeElement::SurfaceInSurfaceMode, { 0.1f, 0.1f, 0.1f, 1.0f} },
            { SchemeElement::SurfaceInComponentMode, { 0.3f, 0.3f, 0.3f, 1.0f} },
            { SchemeElement::SelectedSurface, { 1.0f, 0.2f, 0, 1.0f} },
            { SchemeElement::SelectedVertex, { 1.0f, 0.2f, 0, 1.0f} },
            { SchemeElement::Vertex, { 0.1f, 0.1f, 0.1f, 1.0f} },
            { SchemeElement::Manipulator, { 0.1f, 0.1f, 0.1f, 1.0f} },
            { SchemeElement::SelectedManipulator, { 0.6f, 0.0f, 0, 1.0f} },
            { SchemeElement::ManipulatorSurface, { 0.3f, 0.3f, 0.3f, 1.0f} },
        };

        _schemes[ColourScheme::Light] = SchemeMap
        {
            { SchemeElement::GridText, { 1, 1, 1, 0.8f } },
            { SchemeElement::MajorGrid, { 0.9f, 0.9f, 0.9f, 0.8f } },
            { SchemeElement::MinorGrid, { 0.5f, 0.5f, 0.5f, 0.4f} },
            { SchemeElement::SurfaceInSurfaceMode, { 0.8f, 0.8f, 0.8f, 1.0f} },
            { SchemeElement::SurfaceInComponentMode, { 0.6f, 0.6f, 0.6f, 1.0f} },
            { SchemeElement::SelectedSurface, { 1.0f, 0.5f, 0, 1.0f} },
            { SchemeElement::SelectedVertex, { 1.0f, 0.5f, 0, 1.0f} },
            { SchemeElement::Vertex, { 0.8f, 0.8f, 0.8f, 1.0f} },
            { SchemeElement::Manipulator, { 0.8f, 0.8f, 0.8f, 1.0f} },
            { SchemeElement::SelectedManipulator, { 1.0f, 0.5f, 0, 1.0f} },
            { SchemeElement::ManipulatorSurface, { 1.0f, 0.2f, 0, 1.0f} },
        };
    }

    void setActiveScheme(ColourScheme scheme) override
    {
        _activeScheme = scheme;
    }

    Colour4 getColour(SchemeElement element) override
    {
        return _schemes[_activeScheme][element];
    }
};

module::StaticModule<ColourSchemeManager> _textureToolColourSchemeManager;

}
