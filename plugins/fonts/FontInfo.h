#include "ifonts.h"

#include "GlyphSet.h"
#include <boost/shared_ptr.hpp>

namespace fonts
{

/**
 * Holds information about one specific font.
 * A font consists of one to several resolutions.
 */
class FontInfo :
	public IFontInfo
{
public:
	std::string name;		// The name of the font, e.g. "carleton"
	std::string language;	// The language of this font

	// Three sets of glyphs, one for each resolution
	GlyphSetPtr glyphSets[NumResolutions];

	FontInfo(const std::string& name_, const std::string& language_) :
		name(name_),
		language(language_)
	{}

	const std::string& getName() const
	{
		return name;
	}

	// The language of this font
	const std::string& getLanguage() const
	{
		return language;
	}

	// Returns the glyphset for the specified resolution
	IGlyphSetPtr getGlyphSet(Resolution resolution)
	{
		return glyphSets[resolution];
	}
};
typedef boost::shared_ptr<FontInfo> FontInfoPtr;

} // namespace fonts
