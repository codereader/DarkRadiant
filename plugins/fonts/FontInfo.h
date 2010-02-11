#include "ifonts.h"

#include "GlyphSet.h"
#include <boost/shared_ptr.hpp>

namespace fonts
{

/**
 * Holds information about one specific font.
 * A font consists of one to several resolutions.
 */
struct FontInfo
{
	std::string name;		// The name of the font, e.g. "carleton"
	std::string language;	// The language of this font
	
	// Three sets of glyphs, one for each resolution
	GlyphSetPtr glyphSets[NumResolutions];

	FontInfo(const std::string& name_, const std::string& language_) :
		name(name_),
		language(language_)
	{}
};
typedef boost::shared_ptr<FontInfo> FontInfoPtr;

} // namespace fonts
