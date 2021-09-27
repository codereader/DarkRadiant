#include "itexturetoolcolours.h"

#include <map>

namespace textool
{

class ColourSchemeManager final :
    public IColourSchemeManager
{
private:
    using SchemeMap = std::map<SchemeElement, Colour4b>;
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
        _activeScheme = ColourScheme::Light;

        // Fill in the default scheme values
        _schemes[ColourScheme::Dark] = SchemeMap
        {
            { SchemeElement::MajorGrid, { 0, 0, 0, 0 } }
        };
    }

    void setActiveScheme(ColourScheme scheme)
    {
        _activeScheme = scheme;
    }

    Colour4b getColour(SchemeElement element)
    {
        return _schemes[_activeScheme][element];
    }
};

}
