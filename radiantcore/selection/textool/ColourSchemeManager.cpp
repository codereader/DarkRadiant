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
            { SchemeElement::MinorGrid, { 0.2f, 0.2f, 0.2f, 1.0f} },
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
