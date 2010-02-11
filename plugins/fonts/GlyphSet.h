#ifndef _GLYPHSET_H_
#define _GLYPHSET_H_

#include "GlyphInfo.h"
#include <boost/shared_ptr.hpp>

namespace fonts
{

class GlyphSet;
typedef boost::shared_ptr<GlyphSet> GlyphSetPtr;

// Each D3 font has three resolutions
enum Resolution
{
	Resolution12,
	Resolution24,
	Resolution48,
	NumResolutions
};

namespace q3font
{
	//Default values of Quake 3 sourcecode. Don't change!
	const int GLYPH_COUNT_PER_FONT = 256;
}

// Each font resolution has its own set of glyphs
class GlyphSet
{
public:
	// 12, 24, 48
	Resolution resolution;

	// each set has 256 glyphs
	GlyphInfoPtr glyphs[q3font::GLYPH_COUNT_PER_FONT];

	// the number of textures used by this set
	std::size_t numTextures;

	// Public named constructor
	static GlyphSetPtr createFromDatFile(const std::string& vfsPath);
};
typedef boost::shared_ptr<GlyphSet> GlyphSetPtr;

} // namespace fonts

#endif /* _GLYPHSET_H_ */
